- smooth values 
```
I (81520) sensor_main: temperature 22.687500
I (82490) sensor_main: temperature 54.687500
I (83510) sensor_main: temperature 22.687500
```

- exception on startup 
```
Guru Meditation Error: Core  0 panic'ed (LoadProhibited). Exception was unhandled.

Core  0 register dump:
PC      : 0x400bcd20  PS      : 0x00060330  A0      : 0x8008c573  A1      : 0x3ffdb7f0  
0x400bcd20: httpd_get_global_user_ctx at /home/tothg/own/projects/esp-idf/components/esp_http_server/src/httpd_main.c:192

A2      : 0xde9fc116  A3      : 0x00060323  A4      : 0x000000a5  A5      : 0x000000a5  
A6      : 0x00060023  A7      : 0x00000001  A8      : 0x3ffd155c  A9      : 0xde9fc116  
A10     : 0xde9fc116  A11     : 0xffffffff  A12     : 0xde9fc116  A13     : 0x00000000  
A14     : 0x00000000  A15     : 0x00000001  SAR     : 0x0000000c  EXCCAUSE: 0x0000001c  
EXCVADDR: 0xde9fc136  LBEG    : 0xde9fc116  LEND    : 0x00000000  LCOUNT  : 0x40026f64  
0x40026f64: _xt_handle_exc at /home/tothg/own/projects/esp-idf/components/freertos/FreeRTOS-Kernel/portable/xtensa/xtensa_vectors.S:739



Backtrace: 0x400bcd1d:0x3ffdb7f0 0x4008c570:0x3ffdb820 0x4008ba6b:0x3ffdb850 0x4008b022:0x3ffdb880 0x40127f31:0x3ffdb8b0 0x401285ff:0x3ffdb8f0 0x401286d6:0x3ffdb940 0x400324ce:0x3ffdb970
0x400bcd1d: httpd_get_global_user_ctx at /home/tothg/own/projects/esp-idf/components/esp_http_server/src/httpd_main.c:191

0x4008c570: wss_keep_alive_get_keep_alive at /home/tothg/own/projects/esp/common/ws_keep_alive.c:219

0x4008ba6b: stop_wss_echo_server at /home/tothg/own/projects/esp/common/ws_server.c:184

0x4008b022: handler_on_wifi_disconnect at /home/tothg/own/projects/esp/common/wifi/wifi_main.c:65

0x40127f31: handler_execute at /home/tothg/own/projects/esp-idf/components/esp_event/esp_event.c:137

0x401285ff: esp_event_loop_run at /home/tothg/own/projects/esp-idf/components/esp_event/esp_event.c:601 (discriminator 3)

0x401286d6: esp_event_loop_run_task at /home/tothg/own/projects/esp-idf/components/esp_event/esp_event.c:107 (discriminator 15)

0x400324ce: vPortTaskWrapper at /home/tothg/own/projects/esp-idf/components/freertos/FreeRTOS-Kernel/portable/xtensa/port.c:162
```

