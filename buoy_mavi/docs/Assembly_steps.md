Physalia Instalation Protocol
=============================
## Components
#### GNSS

|**Component name**|**Price**|**Supplier**|
|------------------|---------|------------|
|Drotek DP0601 RTK GNSS (XL F9P)|179,90€|
|JST-GH 6 pins Silicone cable *(1)*||
|Drotek DA910 Multiband GNSS Antenna|99,90€|
|Magnetic antenna pod *(2)*||
|5m SMA(Female) TNC(Female) coaxial antenna cable||

*(1) Shipped with the Drotek DP0601*

*(2) Shipped with the Drotek DA910 Antenna*

#### Microcontroller
|**Component name**|**Price**|**Supplier**|
|------------------|---------|------------|
|ESP32-WROOM-32 board|14,90€|
|USB Type-A to USB Micro-B cable||

#### 4G modem
|**Component name**|**Price**|**Supplier**|
|------------------|---------|------------|
|ZTE MF920U 4G Router|50€|
|USB Type-A to USB Micro-B cable||

#### Power
|**Component name**|**Price**|**Supplier**|
|------------------|---------|------------|
|50W solar panel|50€|
|Solar Charge Controller||
|20Ah Lead Acid battery||
|50cm dual wire 3A cable||
|Ring terminal x2||

#### Case
|**Component name**|**Price**|**Supplier**|
|------------------|---------|------------|
|Water tight case||
|Wood planck (dims)||
|4 screws (diam?)||


## GitHub Repo
- Clone project GitHub [repository](https://github.com/jancelin/physalia)

## Drotek DP0601
### Firmware update

- Download v1.13 or latest [firmware version](https://www.u-blox.com/en/product/zed-f9p-module?file_category=Firmware%2520Update)
- Install [u-center](https://www.u-blox.com/en/product/u-center) (not u-center 2!)
- Upload firmware to Drotek DP0601
	- Plug DP0601 on USB port
	- Open u-center
	- Connect to COM port
	- Go to `Tools` -> `Firmware update`
	- Select your firmware file into `Firmware` box
	- Check and set `Use this baudrate for updtate` to 9600
	- Uncheck all other boxes
	- Click `GO`

### Configuration
#### Upload configuration file

- Open u-center
- Go to `Tools` -> `Receiver Configuration`
- Select the configuration file into `Configuration file` box
- In `Load configuration section` check the box to save config into Flash instead of RAM
- Click `transfer file -> GNSS`

#### Check configuration

- Open u-center
- Go to `View` -> `Configuration View`
- Inside `MSG` section :
	+ Look for `01-07 NAV-PVT` in `Message` field
	+ Make sure `UART1` and `USB` checkboxes are ticked
- Inside `RATE` section :
	+ Make sure `Measurement Frequency` is set to 1Hz
- Inside `PRT` section :
	+ Make sure target `1 - UART1` in/out protocols and baudrate are set to `0+1+5 - UBX+NMEA+RTCM3`, `0+1 - UBX-NMEA` and 115200
- Inside `PRT` section : 
	+ Make sure target `3 - USB` in/out protocols and baudrate are set to `0+1+5 - UBX+NMEA+RTCM3` and `0+1 - UBX-NMEA`  and 115200
- Inside `CFG` section : 
	+ Select `Save current configuration` 
	+ Select `0 - BBR` and `1 - FLASH` devices with `Ctrl+Shift`
	+ Click `Send` to save current configuration to Flash

## 4G modem

- Insert SIM card into the router
- Plug router to a power source
- Power router on
- Wait until router's WiFi access point is up
- Connect to the WiFi access point with a computer using credentials located on router's back
- Acces router's web interface
- Check `Save PIN code`
- Enter SIM card PIN code
- Wait for any update to be done
- Switch the 4G button off
- Go to `Advanced settings`
- In `Other` -> `PIN code`, make sure Save PIN code is checked
- In `Power saving`, select `Short range WiFi` and disable sleep mode
- Back to start page, go to WiFi settings
- Change main SSID to `buoy`
- Change password to `Buoy4321!`
- Set maximum number of connections to 2 (ESP32+User)
- Switch the 4G button back on
- Disconnect from the webpage

## ESP32
- Get your ESP32 board pinout (they vary depending on the manufacturer)
- Get the Drotek DP0601 UART1 port pinout from the [datasheet](https://raw.githubusercontent.com/drotek/datasheets/master/DrotekDoc_0891B08A%20-%20DP0601%20GNSS%20RTK%20(F9P).pdf)
- Cut 2/3 of a flat cable that comes with DP0601
- Strip edges corresponding to 5V, GND, B4 and B5 on DP0601 UART1 port
- Welding
	+ Weld 5V to ESP32 5V
	+ Weld GND to ESP32 GND
	+ Weld B4 to ESP32 I²C SCL
	+ Weld B5 to ESP32 I²C SD0/SDA
- Edit secret.h
	+ Set WiFi credentials (describe)
	+ Set MQTT server info (describe)
	+ Set buoy uuid (describe)
	+ Set Centipède caster info (describe)
- Upload code
	+ Download and open [Arduino IDE](https://www.arduino.cc/en/software)
	+ Install required Arduino ESP32 boards support :
		* Go to `Tools` -> `Boards` -> `Board Manager`
		* Type `ESP32` into search bar
		* Install `esp32` package from `Espressif Systems`
	+ Download required libraries for code compilation :
		* Go to `Sketch` -> `Include library` -> `Manage libraries`
		* Search and install `SparkFun u-blox GNSS Arduino Library` from Sparkfun Electronics
		* Search and install `ArduinoJson` from Benoit Blanchon
		* Search and install `PubSubClient` from Nick O'Leary
	+ Inside Arduino IDE, open repository ESP32 source code located at `buoy/ESP32/NTRIPClient_With_GGA_Callback/NTRIPClient_With_GGA_Callback.ino`

## Case
### Assembly

- Remove but keep aside all foam from inside the case
- Drill antenna hole
- Drill 2 pannel wires holes
- Place cable glands into drilled holes
- Place biggest foam block back inside the case
- **Careful, foam is conductive !**<br>Protect batery electrodes with electrical tape
- Cut a battery size rectangle through the center of the block
- Place battery with its foam block inside the case
- Cut foam around battery electrodes to protect them from shorting through foam
- Cut case size insulation cardboard planck
- Cut a charge controller size rectangle through the middle of that planck
- Paste 3M Dual Lock on charge controller
- Fix its 3M Dual Lock complementary part on the one pasted on the controller
- Remove protective tape from 3M Dual Lock
- Take insulating planck and put controller through its hole at the center
- Place both insulating planck and controller together upon foam block and battery inside the case, in order to paste controller on battery in the exact place of its hole
- Wire controller to battery
- Screw 8mm screws in case top holes so that they maintain insulating plate in place when case is closed

### Onsite setup

- Place case on desired spot
- Connect solar pannel to charge controller through cable glands
- Connect GNSS module to antenna through cable gland
- Connect ESP32 to GNSS module
- Connect 4G router to battery using USB to micro USB cable
- Power 4G router on, and wait until WiFi AP is up
- Connect ESP32 to battery using USB to micro USB cable
- Close case and place solar panel facing towards South


## Buoy

- Place neoprene glue insde bottle support cap holes
- Place bottle caps inside their hole
- Let glue dry partially for 10min
- Tape bottle cap support to maintain caps in place while glue dries
- Let glue dry completely for 48h
- Assemble antenna pod
- Pass antenna pod through bottle support so that incomplete part of bottle cap holes face pod's foot
- Screw antenna on pod
- Screw identical bottles to caps
- Screw antenna cable TNC connector to antenna
