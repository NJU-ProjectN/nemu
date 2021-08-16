/*
 * NEMU (NJU Emulator) sdhost driver.
 *
 * Author:      Zihao Yu <yuzihao@ict.ac.cn>
 *
 * Based on
 *  bcm2835.c by Phil Elwell
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/err.h>
#include <linux/highmem.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/scatterlist.h>
#include <linux/time.h>
#include <linux/workqueue.h>

#include <linux/mmc/host.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/sd.h>

#define SDCMD  0x00 /* Command to SD card              - 16 R/W */
#define SDARG  0x04 /* Argument to SD card             - 32 R/W */
#define SDTOUT 0x08 /* Start value for timeout counter - 32 R/W */
#define SDCDIV 0x0c /* Start value for clock divider   - 11 R/W */
#define SDRSP0 0x10 /* SD card response (31:0)         - 32 R   */
#define SDRSP1 0x14 /* SD card response (63:32)        - 32 R   */
#define SDRSP2 0x18 /* SD card response (95:64)        - 32 R   */
#define SDRSP3 0x1c /* SD card response (127:96)       - 32 R   */
#define SDHSTS 0x20 /* SD host status                  - 11 R/W */
#define SDVDD  0x30 /* SD card power control           -  1 R/W */
#define SDEDM  0x34 /* Emergency Debug Mode            - 13 R/W */
#define SDHCFG 0x38 /* Host configuration              -  2 R/W */
#define SDHBCT 0x3c /* Host byte count (debug)         - 32 R/W */
#define SDDATA 0x40 /* Data to/from SD card            - 32 R/W */
#define SDHBLC 0x50 /* Host block count (SDIO/SDHC)    -  9 R/W */

#define SDCMD_NEW_FLAG			0x8000
#define SDCMD_FAIL_FLAG			0x4000
#define SDCMD_BUSYWAIT			0x800
#define SDCMD_NO_RESPONSE		0x400
#define SDCMD_LONG_RESPONSE		0x200
#define SDCMD_WRITE_CMD			0x80
#define SDCMD_READ_CMD			0x40
#define SDCMD_CMD_MASK			0x3f

#define SDCDIV_MAX_CDIV			0x7ff

#define SDDATA_FIFO_WORDS	16

#define FIFO_READ_THRESHOLD	4
#define FIFO_WRITE_THRESHOLD	4
#define SDDATA_FIFO_PIO_BURST	8

#define PIO_THRESHOLD	1  /* Maximum block count for PIO (0 = always DMA) */

struct nemu_host {
	spinlock_t		lock;
	struct mutex		mutex;

	void __iomem		*ioaddr;
	u32			phys_addr;

	struct mmc_host		*mmc;
	struct platform_device	*pdev;

	int			clock;		/* Current clock speed */
	unsigned int		max_clk;	/* Max possible freq */
	struct sg_mapping_iter	sg_miter;	/* SG state for PIO */
	unsigned int		blocks;		/* remaining PIO blocks */

	struct mmc_request	*mrq;		/* Current request */
	struct mmc_command	*cmd;		/* Current command */
	struct mmc_data		*data;		/* Current data request */
	bool			data_complete:1;/* Data finished before cmd */
	bool			use_sbc:1;	/* Send CMD23 */
};

static void nemu_reset(struct mmc_host *mmc)
{
}

static void nemu_finish_command(struct nemu_host *host);

static void nemu_transfer_block_pio(struct nemu_host *host, bool is_read)
{
	unsigned long flags;
	size_t blksize;

	blksize = host->data->blksz;

	local_irq_save(flags);

	while (blksize) {
		int copy_words;
		size_t len;
		u32 *buf;

		if (!sg_miter_next(&host->sg_miter)) {
			host->data->error = -EINVAL;
			break;
		}

		len = min(host->sg_miter.length, blksize);
		if (len % 4) {
			host->data->error = -EINVAL;
			break;
		}

		blksize -= len;
		host->sg_miter.consumed = len;

		buf = (u32 *)host->sg_miter.addr;

		copy_words = len / 4;

		while (copy_words) {
			int burst_words, words;
			u32 edm;

			burst_words = min(SDDATA_FIFO_PIO_BURST, copy_words);
			edm = (8 << 4);
			if (is_read)
				words = ((edm >> 4) & 0x1f);
			else
				words = SDDATA_FIFO_WORDS - ((edm >> 4) & 0x1f);

			if (words < burst_words) {
				continue;
			} else if (words > copy_words) {
				words = copy_words;
			}

			copy_words -= words;

			while (words) {
				if (is_read)
					*(buf++) = readl(host->ioaddr + SDDATA);
				else
					writel(*(buf++), host->ioaddr + SDDATA);
				words--;
			}
		}
	}

	sg_miter_stop(&host->sg_miter);

	local_irq_restore(flags);
}

static void nemu_transfer_pio(struct nemu_host *host)
{
	bool is_read = (host->data->flags & MMC_DATA_READ) != 0;
	nemu_transfer_block_pio(host, is_read);
}

static
void nemu_prepare_data(struct nemu_host *host, struct mmc_command *cmd)
{
	struct mmc_data *data = cmd->data;
  int flags = SG_MITER_ATOMIC;

	WARN_ON(host->data);

	host->data = data;
	if (!data)
		return;

	host->data_complete = false;
	host->data->bytes_xfered = 0;

  /* Use PIO */
  if (data->flags & MMC_DATA_READ)
    flags |= SG_MITER_TO_SG;
  else
    flags |= SG_MITER_FROM_SG;
  sg_miter_start(&host->sg_miter, data->sg, data->sg_len, flags);
  host->blocks = data->blocks;
}

static void nemu_finish_request(struct nemu_host *host)
{
	struct mmc_request *mrq;

	mrq = host->mrq;

	host->mrq = NULL;
	host->cmd = NULL;
	host->data = NULL;

	mmc_request_done(host->mmc, mrq);
}

static
bool nemu_send_command(struct nemu_host *host, struct mmc_command *cmd)
{
	u32 sdcmd;

	WARN_ON(host->cmd);

	host->cmd = cmd;

	nemu_prepare_data(host, cmd);

	writel(cmd->arg, host->ioaddr + SDARG);

	sdcmd = cmd->opcode & SDCMD_CMD_MASK;

	if (!(cmd->flags & MMC_RSP_PRESENT)) {
		sdcmd |= SDCMD_NO_RESPONSE;
	} else {
		if (cmd->flags & MMC_RSP_136)
			sdcmd |= SDCMD_LONG_RESPONSE;
		if (cmd->flags & MMC_RSP_BUSY) {
			sdcmd |= SDCMD_BUSYWAIT;
		}
	}

	if (cmd->data) {
		if (cmd->data->flags & MMC_DATA_WRITE) {
			sdcmd |= SDCMD_WRITE_CMD;
    }
		if (cmd->data->flags & MMC_DATA_READ)
			sdcmd |= SDCMD_READ_CMD;
	}

	writel(sdcmd | SDCMD_NEW_FLAG, host->ioaddr + SDCMD);

	return true;
}

static void nemu_transfer_complete(struct nemu_host *host)
{
	struct mmc_data *data;

	WARN_ON(!host->data_complete);

	data = host->data;
	host->data = NULL;

	/* Need to send CMD12 if -
	 * a) open-ended multiblock transfer (no CMD23)
	 * b) error in multiblock transfer
	 */
	if (host->mrq->stop && (data->error || !host->use_sbc)) {
		if (nemu_send_command(host, host->mrq->stop)) {
      nemu_finish_command(host);
		}
	} else {
		nemu_finish_request(host);
	}
}

static void nemu_finish_data(struct nemu_host *host)
{
	struct device *dev = &host->pdev->dev;
	struct mmc_data *data;

	data = host->data;

	data->bytes_xfered = data->error ? 0 : (data->blksz * data->blocks);

	host->data_complete = true;

	if (host->cmd) {
		/* Data managed to finish before the
		 * command completed. Make sure we do
		 * things in the proper order.
		 */
		dev_dbg(dev, "Finished early - HSTS %08x\n",
			readl(host->ioaddr + SDHSTS));
	} else {
		nemu_transfer_complete(host);
	}
}

static void nemu_finish_command(struct nemu_host *host)
{
	struct mmc_command *cmd = host->cmd;
  int i;

	if (cmd->flags & MMC_RSP_PRESENT) {
		if (cmd->flags & MMC_RSP_136) {
			for (i = 0; i < 4; i++) {
				cmd->resp[3 - i] =
					readl(host->ioaddr + SDRSP0 + i * 4);
			}
		} else {
			cmd->resp[0] = readl(host->ioaddr + SDRSP0);
		}
	}

	if (cmd == host->mrq->sbc) {
		/* Finished CMD23, now send actual command. */
		host->cmd = NULL;
		if (nemu_send_command(host, host->mrq->cmd)) {
			if (host->data) {
        // start PIO right now
        for (i = 0; i < host->data->blocks; i ++) {
          nemu_transfer_pio(host);
        }

        nemu_finish_data(host);
      }

      nemu_finish_command(host);
		}
	} else if (cmd == host->mrq->stop) {
		/* Finished CMD12 */
		nemu_finish_request(host);
	} else {
		/* Processed actual command. */
		host->cmd = NULL;
		if (!host->data) {
			nemu_finish_request(host);
    }
		else if (host->data_complete) {
			nemu_transfer_complete(host);
    }
	}
}

static void nemu_request(struct mmc_host *mmc, struct mmc_request *mrq)
{
	struct nemu_host *host = mmc_priv(mmc);
	struct device *dev = &host->pdev->dev;

	/* Reset the error statuses in case this is a retry */
	if (mrq->sbc)
		mrq->sbc->error = 0;
	if (mrq->cmd)
		mrq->cmd->error = 0;
	if (mrq->data)
		mrq->data->error = 0;
	if (mrq->stop)
		mrq->stop->error = 0;

	if (mrq->data && !is_power_of_2(mrq->data->blksz)) {
		dev_err(dev, "unsupported block size (%d bytes)\n",
			mrq->data->blksz);

		if (mrq->cmd)
			mrq->cmd->error = -EINVAL;

		mmc_request_done(mmc, mrq);
		return;
	}

	mutex_lock(&host->mutex);

	WARN_ON(host->mrq);
	host->mrq = mrq;

	host->use_sbc = !!mrq->sbc && host->mrq->data &&
			(host->mrq->data->flags & MMC_DATA_READ);
	if (host->use_sbc) {
		if (nemu_send_command(host, mrq->sbc)) {
      nemu_finish_command(host);
		}
	} else if (mrq->cmd && nemu_send_command(host, mrq->cmd)) {
		if (host->data) {
      int i;
      // start PIO right now
      for (i = 0; i < host->data->blocks; i ++) {
        nemu_transfer_pio(host);
      }
      nemu_finish_data(host);
    }

    nemu_finish_command(host);
	}

	mutex_unlock(&host->mutex);
}

static void nemu_set_ios(struct mmc_host *mmc, struct mmc_ios *ios)
{
}

static const struct mmc_host_ops nemu_ops = {
	.request = nemu_request,
	.set_ios = nemu_set_ios,
	.hw_reset = nemu_reset,
};

static int nemu_add_host(struct nemu_host *host)
{
	struct mmc_host *mmc = host->mmc;
	struct device *dev = &host->pdev->dev;
	int ret;

	if (!mmc->f_max || mmc->f_max > host->max_clk)
		mmc->f_max = host->max_clk;
	mmc->f_min = host->max_clk / SDCDIV_MAX_CDIV;

	mmc->max_busy_timeout = ~0 / (mmc->f_max / 1000);

	dev_dbg(dev, "f_max %d, f_min %d, max_busy_timeout %d\n",
		mmc->f_max, mmc->f_min, mmc->max_busy_timeout);

	/* host controller capabilities */
	mmc->caps |= MMC_CAP_SD_HIGHSPEED | MMC_CAP_MMC_HIGHSPEED |
		     MMC_CAP_NEEDS_POLL | MMC_CAP_HW_RESET | MMC_CAP_ERASE |
		     MMC_CAP_CMD23;

	spin_lock_init(&host->lock);
	mutex_init(&host->mutex);

	mmc->max_segs = 128;
	mmc->max_req_size = 524288;
	mmc->max_seg_size = mmc->max_req_size;
	mmc->max_blk_size = 1024;
	mmc->max_blk_count =  65535;

	/* report supported voltage ranges */
	mmc->ocr_avail = MMC_VDD_32_33 | MMC_VDD_33_34;

	ret = mmc_add_host(mmc);
	if (ret) {
		return ret;
	}

	dev_info(dev, "loaded - DMA %s\n", "disabled");

	return 0;
}

static int nemu_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct resource *iomem;
	struct nemu_host *host;
	struct mmc_host *mmc;
	const __be32 *regaddr_p;
	int ret;

	dev_dbg(dev, "%s\n", __func__);
	mmc = mmc_alloc_host(sizeof(*host), dev);
	if (!mmc)
		return -ENOMEM;

	mmc->ops = &nemu_ops;
	host = mmc_priv(mmc);
	host->mmc = mmc;
	host->pdev = pdev;
	spin_lock_init(&host->lock);

	iomem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	host->ioaddr = devm_ioremap_resource(dev, iomem);
	if (IS_ERR(host->ioaddr)) {
		ret = PTR_ERR(host->ioaddr);
		goto err;
	}

	/* Parse OF address directly to get the physical address for
	 * DMA to our registers.
	 */
	regaddr_p = of_get_address(pdev->dev.of_node, 0, NULL, NULL);
	if (!regaddr_p) {
		dev_err(dev, "Can't get phys address\n");
		ret = -EINVAL;
		goto err;
	}

	host->phys_addr = be32_to_cpup(regaddr_p);

	host->max_clk = 1000000; //clk_get_rate(clk);

	ret = mmc_of_parse(mmc);
	if (ret)
		goto err;

	ret = nemu_add_host(host);
	if (ret)
		goto err;

	platform_set_drvdata(pdev, host);

	dev_dbg(dev, "%s -> OK\n", __func__);

	return 0;

err:
	dev_dbg(dev, "%s -> err %d\n", __func__, ret);
	mmc_free_host(mmc);

	return ret;
}

static int nemu_remove(struct platform_device *pdev)
{
	struct nemu_host *host = platform_get_drvdata(pdev);

	mmc_remove_host(host->mmc);

	mmc_free_host(host->mmc);
	platform_set_drvdata(pdev, NULL);

	return 0;
}

static const struct of_device_id nemu_match[] = {
	{ .compatible = "nemu-sdhost" },
	{ }
};
MODULE_DEVICE_TABLE(of, nemu_match);

static struct platform_driver nemu_driver = {
	.probe      = nemu_probe,
	.remove     = nemu_remove,
	.driver     = {
		.name		= "sdhost-nemu",
		.of_match_table	= nemu_match,
	},
};
module_platform_driver(nemu_driver);

MODULE_ALIAS("platform:sdhost-nemu");
MODULE_DESCRIPTION("NEMU SDHost driver");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Zihao Yu");
