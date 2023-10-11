Short term
==========

- víz
  - ellenállás üzemanyaghoz
  - átkötés vízhez
  - csatlakozó-aljzat vízhez 
- kijelző 
  - kábelezésremm
  - új kinézet
  - 
- logger
  - meglévő hardveren
  - SD kártya
  - gyroscope (MPU6050)

Before go live
==============

+ calculate displayed values from raw adc
+ change values displayed
+ fluid level hardware
+ fluid level measurement
+ own 3.3V regulator
+ shared SPI bus on display
+ turn display off (timer/touch) / on (touch irq)
+ schematics
+ can transceiver standby?
+ 160MHz for drawing display

- fluid level power source
- adc read on timer only
- round up/down when displaying data

Long term plans
===============

- reduce consumption
  ```
  setCpuFrequencyMhz(160);
  adc_power_off();
  ```
- data logger to SD card
- configuration via ISO-11783 commands
- measure declination
- configuration on display
    - dimmer
    - off timer

Hardware
========
