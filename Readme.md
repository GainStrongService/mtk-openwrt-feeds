# Brief introduction for using MediaTek released OpenWrt SDK

## 1. Build system setup

- Please refer to https://openwrt.org/docs/guide-developer/toolchain/install-buildsystem

## 2. Clone vanilla OpenWrt

Currently two release branches are supported:

1. 21.02
    This is the current in-use branch

   ```bash
   git clone -b openwrt-21.02 https://git.openwrt.org/openwrt/openwrt.git
   ```

2. master (will be 24.0x branch later)
   The next version in development

   ```bash
   git clone https://git.openwrt.org/openwrt/openwrt.git
   ```

## 3. Add MediaTek OpenWrt feed

```bash
cd openwrt
echo "src-git mtk_openwrt_feed https://git01.mediatek.com/openwrt/feeds/mtk-openwrt-feeds" >> feeds.conf.default
./scripts/feeds update -a
./scripts/feeds install -a
```

## 4. Apply MediaTek OpenWrt files and patches

1. 21.02 branch

   ```bash
   cp -af ./feeds/mtk_openwrt_feed/21.02/files/* .
   cp -af ./feeds/mtk_openwrt_feed/tools .
   for file in $(find ./feeds/mtk_openwrt_feed/21.02/patches-base -name "*.patch" | sort); do patch -f -p1 -i ${file}; done
   for file in $(find ./feeds/mtk_openwrt_feed/21.02/patches-feeds -name "*.patch" | sort); do patch -f -p1 -i ${file}; done
   ```

2. master branch

   ```bash
   cp -af ./feeds/mtk_openwrt_feed/master/files/* .
   for file in $(find ./feeds/mtk_openwrt_feed/master/patches-base -name "*.patch" | sort); do patch -f -p1 -i ${file}; done
   ```

## 5. Configuration

```bash
make menuconfig
```

1. 21.02 branch

   ```text
   Target System -> MediaTek Ralink ARM
   Subtarget -> MT7981 / MT7986 / MT7988
   Target Profile -> select as needed
   ```

2. master branch

   ```text
   Target System -> MediaTek Ralink ARM
   Subtarget -> Filogic 8x0 (MT798x)
   Target Profile -> select as needed
   ```

## 6. Build

```bash
make V=s -j$(nproc)
```
