## LILYGO T-Pcie + SIM7600G

![Lilygo T-pcie + sim7600g](https://github.com/Xinyuan-LilyGO/LilyGo-T-PCIE/raw/master/image/pins.jpg)

https://github.com/Xinyuan-LilyGO/LilyGo-T-PCIE/tree/master

* Material:
  - [ESP32 with Pcie + bat: Lilygo T-pcie](https://www.tinytronics.nl/shop/en/development-boards/microcontroller-boards/with-wi-fi/lilygo-t-pcie-v1.2-axp2101-esp32-wrover-16mb)
  - [LTE 4G: SIM7600g](https://www.tinytronics.nl/shop/en/communication-and-signals/wireless/gps/modules/lilygo-ttgo-t-pcie-sim7600g-h-expansion-module)
  - [F9P: drotek DP0601](https://store-drotek.com/891-rtk-zed-f9p-gnss.html)
  - [relay: 5V 1-channel high-active](https://www.tinytronics.nl/shop/en/switches/relays/5v-relay-1-channel-high-active)
  - [battery: Li-Po Battery 3.7V 2000mAh](https://www.tinytronics.nl/shop/en/power/batteries/li-po/li-po-accu-3.7v-2000mah)
  

### Arduino IDE

![image](https://github.com/jancelin/physalia/assets/6421175/4a6827f4-2eff-4e9a-9c4c-cf8777e0086d)

### Install Wine for U-Center

  sudo dpkg --add-architecture i386 
  sudo mkdir -pm755 /etc/apt/keyrings
  sudo wget -O /etc/apt/keyrings/winehq-archive key https://dl.winehq.org/wine-builds/winehq.key
  sudo wget -NP /etc/apt/sources.list.d/ https://dl.winehq.org/wine-builds/ubuntu/dists/jammy/winehq-jammy.sources
  sudo apt update
  sudo apt install --install-recommends winehq-stable

// Config Wine pour redirection de port du ttyACM0 -> com33 ( détectable dans u-center )

ls -l /dev
cd .wine/dosdevices/
ls
ln -s /dev/ttyACM0 com33


### Download u-Center
https://content.u-blox.com/sites/default/files/2022-05/UBX_F9_100_HPG132.df73486d99374142f3aabf79b7178f48.bin


