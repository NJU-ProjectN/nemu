
# NEMU sdhost驱动

本驱动裁剪自`linux/drivers/mmc/host/bcm2835.c`, 去除了DMA和中断, 改成直接轮询, 处理器无需支持DMA和中断即可运行.

## 使用方法

* 将本目录下的`nemu.c`复制到`linux/drivers/mmc/host/`目录下
* 在`linux/drivers/mmc/host/Makefile`中添加一行`obj-y += nemu.o`
* 在menuconfig中取消`General setup -> Initial RAM filesystem and RAM disk (initramfs/initrd) support`
* 在menuconfig中选中`Device Drivers -> MMC/SD/SDIO card support`
* 在dts中加入以下节点
```
/ {
  soc {
    sdhci: mmc {
      compatible = "nemu-sdhost";
      reg = <0x0 0xa3000000 0x0 0x1000>;
    };
  };

  chosen {
    bootargs = "root=/dev/mmcblk0p1 rootfstype=ext4 ro rootwait earlycon";
  };
};
```

## 在没有中断的处理器上访问SD卡

访问真实的SD卡需要等待一定的延迟, 这需要处理器的中断机制对内核支持计时的功能.
在没有中断机制的处理器上, 我们可以修改内核的部分代码, 使得无需等待这些延迟,
来达到确定性可重复的仿真效果.

具体只需修改以下文件:
```diff
--- linux/drivers/mmc/core/block.c
+++ linux/drivers/mmc/core/block.c
@@ -983,6 +983,7 @@ static int card_busy_detect(struct mmc_card *card, unsigned int timeout_ms,
 	int err = 0;
 	u32 status;
 
+  return err;
 	do {
 		bool done = time_after(jiffies, timeout);
 
--- linux/drivers/mmc/core/core.h
+++ linux/drivers/mmc/core/core.h
@@ -64,6 +64,7 @@ void mmc_set_initial_state(struct mmc_host *host);
 
 static inline void mmc_delay(unsigned int ms)
 {
+  return;
 	if (ms <= 20)
 		usleep_range(ms * 1000, ms * 1250);
 	else
```

注意: 上述修改仅能用于模拟和仿真, 修改后将不能在真实的SD卡上运行!!!
