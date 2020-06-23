# PCB with KiCad

## Soil Sensor v2

This is the (currently) final version without any unneded HW. It is optimised to make it easy to esamble with only the programing pins and the battery holder not being SMD components.

TBD overview picture

- [Aisler PCB project](https://aisler.net/p/DFIQTREA)
- [SoilSensor.sch](SoilSensor_v2.sch)
  - ![Schematics Preview](pictures/SoilSensor%20v2%20Schematics.png)
- [SoilSensor.kicad_pcb](SoilSensor_v2.kicad_pcb)
- [SVG for soil sonde](pictures/soilSonde.svg)
  - edited with [Inkscape](https://inkscape.org)
  - imported to KiCad with [svg2shenzhen](https://github.com/badgeek/svg2shenzhen) plugin.
  - ![Sonde Preview](pictures/soilSonde.svg)


## Soil Sensor v1

This was the first version on STM32. It has multiple debug options so more complex than needed. Battery is connected with Dupont cables.

TBD overview picture

- [Aisler PCB project](https://aisler.net/p/FEKNZTQA)
- [SoilSensor.sch](SoilSensor.sch)
  - ![Schematics Preview](pictures/SoilSensor%20v1%20Schematics.png)
- [SoilSensor.kicad_pcb](SoilSensor.kicad_pcb)
- [SVG for soil sonde](pictures/soilSonde.svg)
  - edited with [Inkscape](https://inkscape.org)
  - imported to KiCad with [svg2shenzhen](https://github.com/badgeek/svg2shenzhen) plugin.
  - ![Sonde Preview](pictures/soilSonde.svg)
- BUGS:
  - The boot input is missing a pull-up so sometimes the PCB boots into the bootloader after power-up. See overview picture on how to manually add one.
