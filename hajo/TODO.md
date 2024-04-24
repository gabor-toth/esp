Short term
==========

- víz
    - ellenállás víz visszajövőhöz?
- kijelző
    - kábelezés
    - új kinézet
    - https://www.youtube.com/watch?v=6uKf5Bj0xcc&ab_channel=SquareLine
- logger
- battery
    - nyákon bevágás a csavar helyén

- hűtő
    - ASR-03DA, ASR-02DD Szilárdtest relé https://www.hestore.hu/prod_10035237.html
    - relé 3v SMT https://www.hestore.hu/prod_10039590.html
    - SSR DIP8-ban https://www.hestore.hu/prod_10028866.html
    - IR530n https://electronics.stackexchange.com/questions/393066/how-to-control-12v-from-5v-using-transistor

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

- data logger to SD card
- configuration via ISO-11783 commands
- configuration on display
    - dimmer
    - off timer

Hardware
========
