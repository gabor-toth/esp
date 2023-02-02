Before go live
==============

+ calculate displayed values from raw adc
+ change values displayed
+ fluid level hardware
+ fluid level measurement
+ own 3.3V regulator
+ shared SPI bus on display
+ turn display off (timer/touch) / on (touch irq)

- fluid level power source
- adc read on timer only
- schematics
- round up/down when displaying data
- can transceiver standby?
- 160MHz for drawing display

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
