# Soil sensor for LoraWan

This sensor uses a custom PCB to measure soil and send via LoraWan when the value changes enough. It can work on a CR123 battery for 20 years (self-discharge of the battery being the main factor). It sends the battery voltage and (optionally) the temp of the MCU (using factory calibration).

For each new detected device it generates unique LoraWan keys and upload them to flash. This way the same firmware can be used for multiple instances of the same board.

Download commands might be used to query or modify the configuration. See the commands section bellow.

## Bill of Materials
It can be built for ca 15€ ussing the following parts:
- STM32 MCU - any of the following:
  - STM32L412KB (3 €) - prefered: 128KB RAM for a future firmware OTA update
    - STM32L412KBT6 128K Flash,  8K RAM, LQFP-32
  - STM32L412K8 (2.5 €) - same as STM32L412KB but only 64K Flash
    - STM32L412K8T6 64K Flash,  8K RAM, LQFP-32
  - STM32L010K8 (1.5 €) - cheaper MCO+, slower, less memory, not tem and slightly higher consumption
    - STM32L412K8T6 64K Flash,  8K RAM, LQFP-32 
- RFM95W (3.5 € via Aliexpress, 3x this price if bought in Europe)
- PCB from [Aisler](https://aisler.net/p/DFIQTREA) (4.4 € when ordering 12) - design in [KiCad](KiCad) folder
- RC123a battery and holder (2€ via Aliexpress, 2x this price if bought in Europe)
- 3D-printed box - design in [cage](cage) folder
- some SMT components (1 €) - see schematics in [KiCad](KiCad) folder

## References

- Moisture sensors
  - https://hackaday.io/project/161994-mesh-network-soil-moisture-sensor
  - https://flashgamer.com/blog/comments/testing-capacitive-soil-moisture-sensors
  - https://www.thethingsnetwork.org/labs/story/a-cheap-stm32-arduino-node
- Touch sensors
  - using two pins: https://github.com/PaulStoffregen/CapacitiveSensor
    - this is what it used here
  - STM32 - https://github.com/arpruss/ADCTouchSensor
    - based on https://github.com/martin2250/ADCTouch


## Downlink commands

Downlink commands are used to query and/or modify the device settings without having to reprogram the flash. Each command uses one byte plus optional data depending on the command. The commands are implemtented in the `lorawan_process_command` at [lorawan.cpp](src/lorawan.cpp#L188)

Commands will be queued and transferred to the device after the next uplink transfer. Multiple commands can be queued but be carefull not to exceed to the watdown timeout or the device will get reset.

Commands:
- 0x00 - READ configuration
  - Description: Returns the configuration of the device. See [device_config_t](src/config.h#L27) for more details on the struct
  - Additional downlink bytes: none
  - Example output: 02 03 3C00 05 3C 6400 5802 01 0A 05 FF
- 0x01 - Reset config to the default
  - Additional downlink bytes: none
- 0x02 - Sets the configuration
  - Additional downlink bytes: config as returned by the 0x00 command
  - Example input: 02 02033C00053C64005802010A05FF
- 0x05 - Read Device UID
  - Additional downlink bytes: none
  - Output: 12 bytes with STM32 UID
- 0x06 - Read firmware GIT commit
  - Additional downlink bytes: none
  - Output: ASCII string
- 0x07 - Read firmware BUILD date
  - Additional downlink bytes: none
  - Output: ASCII string

## Firmware

This project requires [Platformio](https://platformio.org/) and the [STM32 Cube Programmer](https://www.st.com/en/development-tools/stm32cubeprog.html). Both need to be installed and available in the PATH.

Keys for each detected device will be stored under `config/devices.ini`. Before you run for the first time please create the file and include your app EUI. You can create this by registering a new app at the [TTN](https://console.thethingsnetwork.org/applications) console. The file should have:
```
[global]
appeui = <your app EUI>
```
If you run without creating the file first then the programmer will create a dummy APPEUI which you will need to modify. You can keep the config folder into its own git repository. Please keep it private!

For programming either use the platformio UI or any of the following CLI commands:
- `pio run -t upload -e soilsensor_v2_L4` - PCB v2, STM32 L4 128 KB
- `pio run -t upload -e soilsensor_v2_L4s` - PCB v2, STM32 L4 64 KB
- `pio run -t upload -e soilsensor_v1_L4` - PCB v1, STM32 L4 128 KB
- `pio run -t upload -e soilsensor_v1_L0` - PCB v1, STM32 L0

Unset the IWDG_STOP user congliguration flag to let the device deep sleep without the Watchdog reseting the device. This will be done automatically in the future.

## ToDos
- Add some pictures
- keys upload tool
  - parse target memory instead of requiring parameter
  - parse option registers
  - more documentation
- Firmware OTA update

