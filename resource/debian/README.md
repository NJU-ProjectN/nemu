
# riscv64 debian镜像制作

制作需要`qemu-riscv64-static`, 建议在debian 10或ubuntu 19.04的系统(可尝试使用docker)中进行操作.

* 创建空镜像和分区
```
dd if=/dev/zero of=debian.img bs=1G count=16  # 此处镜像大小为16GB
sudo cfdisk debian.img # 可创建两个分区, 第一个分区12GB作为rootfs, 第二个分区4GB作为swap
sudo losetup --partscan --show --find debian.img # 将debian.img作为loop设备
ls /dev/loop0* # 此时应该能看到/dev/loop0p1和/dev/loop0p2两个分区
```

* 创建ext4和swap文件系统
```
sudo mkfs.ext4 /dev/loop0p1
sudo mkswap /dev/loop0p2
```

* 挂载ext4分区
```
sudo mount /dev/loop0p1 /mnt
```

* 安装debian base system.
下面两条命令的操作来自[debian社区的安装指南](https://wiki.debian.org/RISC-V#debootstrap).
```
sudo apt-get install debootstrap qemu-user-static binfmt-support debian-ports-archive-keyring
sudo debootstrap --arch=riscv64 --keyring /usr/share/keyrings/debian-ports-archive-keyring.gpg --include=debian-ports-archive-keyring unstable /mnt http://deb.debian.org/debian-ports
```
若要安装x86系统, 则输入
```
sudo debootstrap --arch=i386 --keyring /usr/share/keyrings/debian-archive-keyring.gpg --include=debian-archive-keyring stable /mnt http://deb.debian.org/debian
```

* 进入镜像
```
sudo chroot /mnt /bin/bash
```
此时实际上是通过`qemu-riscv64-static`来执行镜像中的riscv64可执行文件.

* 安装所需工具(根据实际情况选择)
```
apt-get update
apt-get install systemd
apt-get install gcc build-essential
apt-get install tmux libreadline-dev
apt-get install net-tools openssh-server
# fix long delay of openssh server
apt-get install haveged
apt-get install sbt
```

* 修复`NO_PUBKEY E852514F5DF312F6`错误, 见[这里](https://www.reddit.com/r/RISCV/comments/sn0cph/sipeed_debian_the_following_signatures_couldnt_be/)
```
curl https://www.ports.debian.org/archive_2022.key | apt-key add -
```

* 同步时间
```
apt-get install ntpdate
ntpdate 0.asia.pool.ntp.org
```

* 在`/etc/fstab`中添加swap分区
```
/dev/mmcblk0p2 none swap sw 0 0
```

* 添加/root/目录的写和执行权限, 使得host上的普通用户可以访问
```
chmod a+w,a+x /root
```

* 在/root/目录下提前写入所需的测试文件, 如hello.c等.

* 在/root/.bashrc中添加如下内容, 可以实现登录后自动运行命令(根据实际情况修改测试的命令):
```
TMP_DIR=/run/mytest

cmd=(
# enbale swap
  "swapon -a"

# show system information
  "uname -a"
  "cat /etc/issue"
  "cat /proc/cpuinfo"
  "df -ah"
  "free -h"

# show time
  "date"
  "uptime"

# create and switch to tmp directory
  "mkdir $TMP_DIR"
  "cd $TMP_DIR"

# compile and run hello
  "ls /root"
  "ls /root/hello"
  "cat /root/hello/hello.c"
  "gcc -time /root/hello/hello.c -o $TMP_DIR/hello"
  "ls -lh $TMP_DIR"
  "$TMP_DIR/hello"

# compile and run x86-nemu
  "ls /root/nemu"
  "cp -r /root/nemu $TMP_DIR"
  "export NEMU_HOME=$TMP_DIR/nemu"
  "make -C $TMP_DIR/nemu ISA=x86"
  "ls -lh /root/nemu-prog"
  "file /root/nemu-prog/amtest-x86-nemu.elf"
  "$TMP_DIR/nemu/build/x86-nemu --batch --mainargs=h /root/nemu-prog/amtest-x86-nemu.bin"
  "file /root/nemu-prog/microbench-x86-nemu.elf"
  "$TMP_DIR/nemu/build/x86-nemu --batch --mainargs=test /root/nemu-prog/microbench-x86-nemu.bin"

# compile and run riscv64-nemu
  "make -C $TMP_DIR/nemu clean"
  "make -C $TMP_DIR/nemu ISA=riscv64"
  "$TMP_DIR/nemu/build/riscv64-nemu --batch /root/nemu-prog/linux-hello-riscv64-nemu.bin"
)

prompt="`whoami`@`hostname`:`pwd`#"

echo -e "\n============ Now running preset commands =============\n"

for ((i = 0; i < ${#cmd[@]}; i++)); do
  c=${cmd[$i]}
  echo "$prompt $c"
  $c
  echo ""
done

echo -e "\n============ End of preset commands =============\n"

/root/nemutrap/good-trap
```

* 若在不方便输入的环境(如NEMU, verilator仿真等)中测试, 可采用如下两种方式的其中一种, 避免登录时输入
  * 通过紧急模式登录
```
cd /lib/systemd/system
# 通过紧急模式登录, 不启动非必须的服务, 节省将近一半的登录时间
ln -sf emergency.target default.target
# 跳过登录提示符, 直接运行bash
vim emergency.service
  -ExecStart=-/lib/systemd/systemd-sulogin-shell emergency
  +ExecStart=-/bin/bash
```
  * 免密码登录, 见[这里](https://superuser.com/questions/969923/automatic-root-login-in-debian-8-0-console-only)
```
cd /lib/systemd/system
vim serial-getty@.service
  -ExecStart=-/sbin/agetty -o '-p -- \\u' --keep-baud 115200,57600,38400,9600 %I $TERM
  +ExecStart=-/sbin/agetty -a root --keep-baud 115200,57600,38400,9600 %I $TERM
```

* 退出并卸载镜像
```
exit  # 之前通过`chroot`方式进入
sudo umount /mnt  # 记得卸载! 在未卸载镜像的情况下通过可写方式再次打开`debian.img`(如作为qemu的文件系统), 镜像将会损坏!
sudo losetup -d /dev/loop0  # 删除loop设备
```

* 修改`nemu/src/device/sdcard.c`中`init_sdcard()`中打开的镜像文件路径, 即可使用制作的镜像.
在i9-9900k上测试, 约90s后看到debian的登录提示符.

* 当以可写方式启动镜像时, NEMU遇到错误或通过Ctrl+C直接退出NEMU时, 可能会损坏镜像的崩溃一致性, 此时可以通过fsck命令修复分区.

* 更多命令可参考[这里](https://github.com/carlosedp/riscv-bringup/blob/master/Debian-Rootfs-Guide.md).
