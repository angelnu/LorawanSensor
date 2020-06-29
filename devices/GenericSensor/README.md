# Generic Sensor PCB

This device exposed all available GPIOs from the 32 pin STM32 so external sensors can be attached. Multiple use cases are listed bellow.

TBD picture overview installed

## Versions of the PCB

### Generic Sensor v1

This is the (currently) final version without any unneded HW. It is optimised to make it easy to esamble with only the programing pins and the battery holder not being SMD components.

TBD overview picture painted

- [Aisler PCB project](https://aisler.net/p/YFVFQBHD)
- [SoilSensor.sch](KiCad/GenericSensor_v1.sch)
  - ![Schematics Preview](pictures/GenericSensor_v1_schematics.png)
- [SoilSensor.kicad_pcb](KiCad/GenericSensor_v1.kicad_pcb)
  - ![PCB TBD](TBD.jpg)

## Use Cases

### Distance Sensor

Used to detect when a car is parked in a parking lot.

- Firmware:
  For programming either use the platformio UI or any of the following CLI commands:
  - `pio run -t upload -e distance_v1` - PCB v2, STM32 L4
- [3D Printed cage](cages):
  - ![Cage Preview TBD](TBD.jpg)
- BUGS:
  - None so far