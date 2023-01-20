Before go live
==============

+ calculate displayed values from raw adc
+ change values displayed
+ fluid level hardware
+ fluid level measurement
+ own 3.3V regulator
+ shared SPI bus on display
+ turn display off (timer/touch) / on (touch irq)

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

R 120
C 10uF +-
jumper
