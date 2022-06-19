Frontend
========

- ~~routing~~
    - ~~state~~
    - ~~highlight selected route~~
- state
    - manual display/change
- admin
    - set names
    - reset esp
    - manage wifi connections
    - manage programs
- error handling (no connection, etc)
- ~~selective value settings (angular)~~
- ~~display esp's clock~~

Backend
=======

- reset esp
- ~~program admin~~
- program logic
- ~~SNTP~~
- ~~httpd: return index.html for unknown files~~
- logic
    - ~~delay pump turn on after level 4 drop~~
- show next program time
- 'get prog' should return time
- store and retrieve logs (esp_log_set_vprintf(vprintf_like_t func))
- run program with percentage
- set percentage on program
- set overall percentage (?)
- overall program disable
- websocket?
- gpio_logic change main pump state: notify program logic
- program_logic: suspend/resume on empty tank
- pump manual change: re-evaluate state

Hardware
========

- physical button
- lcd display?
- esp32 wifi antenna (needs soldering, see 3rd picture
  on https://marksbench.com/electronics/esp32-cam-antenna-workaround/)
- microtik ap outside