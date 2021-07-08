# Build and deploy rtkbase_rover

```
wget https://raw.githubusercontent.com/jancelin/rtkbase/dev_rtkrover/tools/install.sh
chmod +x install.sh
sudo ./install.sh --dependencies --rtklib --rtkbase-repo --gpsd-chrony -unit-files --detect-usb-gnss --configure-gnss --start-services
```

* Go grab a coffee, it's gonna take a while. The script will install the needed softwares, and 
  * if you use a Usb-connected U-Blox ZED-F9P receiver, it'll be detected and set to work as a base station. 
  * If you don't use a F9P: 
    * you will have to configure your receiver manually and choose the correct port from the settings page.
    * you can use a f9p streaming data test from a Centipede Base (CT2)

* Open a web browser to http://ip_of_your_sbc (the script will try to show you this ip address) or http://basegnss.local.

## Use a f9p streaming data test from a Centipede Base (CT2)

* Open rtklib config file and modify
