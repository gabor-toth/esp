Short term
==========

- logger
    - correction
- hűtő
    - ASR-03DA, ASR-02DD Szilárdtest relé https://www.hestore.hu/prod_10035237.html
    - relé 3v SMT https://www.hestore.hu/prod_10039590.html
    - SSR DIP8-ban https://www.hestore.hu/prod_10028866.html
    - IR530n https://electronics.stackexchange.com/questions/393066/how-to-control-12v-from-5v-using-transistor

Nyák
====

- kijelző
    - BC557 helyett MOSFET, hogy ne legyen feszültségesés?
- battery
    - nyákon bevágás a csavar helyén
- logger
    - debugger csatlakozó

Before go live
==============

- adc read on timer only
- round up/down when displaying data

Long term plans
===============

- data logger to SD card
- configuration via ISO-11783 commands
- configuration on display
    - dimmer
    - off timer
- configuration on logger
    - correction for pitch/

```
n2k_recv W (90652) transport_base: Poll timeout or error, errno=Bad file number, fd=1073723100, timeout_ms=100
n2k_recv E (90652) transport_ws: Error write header

assert failed: tlsf_free tlsf.c:1120 (!block_is_free(block) && "block already marked as free")
HINT: CORRUPT HEAP: heap metadata corrupted resulting in TLSF malfunction.
Make sure you are not making out of bound writing on the memory you allocate in your application.
Make sure you are not writing on freed memory.
For more information run 'idf.py docs -sp api-reference/system/heap_debug.html'.


Backtrace: 0x400258cf:0x3fff2540 0x400328bd:0x3fff2570 0x4003daea:0x3fff25a0 0x4003ae76:0x3fff26c0 0x4003aba8:0x3fff26f0 0x40026199:0x3fff2720 0x4003db85:0x3fff2750 0x400a56a5:0x3fff2780 0x400a59d9:0x3fff27d0 0x400a6b8c:0x3fff2820 0x400a6bed:0x3fff2850 0x4008f48f:0x3fff2880 0x4008f529:0x3fff28f0 0x4009184c:0x3fff2920 0x40091f86:0x3fff2980 0x4008bec5:0x3fff29c0 0x40096e92:0x3fff29f0 0x40099611:0x3fff2a20 0x4008c3c5:0x3fff2a60 0x40033c6e:0x3fff2a90
0x400258cf: panic_abort at /home/tothg/own/projects/esp-idf/components/esp_system/panic.c:472
0x400328bd: esp_system_abort at /home/tothg/own/projects/esp-idf/components/esp_system/port/esp_system_chip.c:93
0x40026199: heap_caps_free at /home/tothg/own/projects/esp-idf/components/heap/heap_caps.c:393
0x400a56a5: esp_websocket_client_error at /home/tothg/own/projects/esp/hajo/components/espressif__esp_websocket_client/esp_websocket_client.c:250
0x400a59d9: esp_websocket_client_send_with_exact_opcode at /home/tothg/own/projects/esp/hajo/components/espressif__esp_websocket_client/esp_websocket_client.c:578
0x400a6b8c: esp_websocket_client_send_with_opcode at /home/tothg/own/projects/esp/hajo/components/espressif__esp_websocket_client/esp_websocket_client.c:1234
0x400a6bed: esp_websocket_client_send_text at /home/tothg/own/projects/esp/hajo/components/espressif__esp_websocket_client/esp_websocket_client.c:1183
0x4008f48f: EspSigK::sendDeltaSet(DeltaSet&) at /home/tothg/own/projects/esp/hajo/main/devices/signalk/EspSigK.cpp:614 (discriminator 1)
0x4008f529: DeltaSet::send(EspSigK&) at /home/tothg/own/projects/esp/hajo/main/devices/signalk/EspSigK.cpp:632
0x4009184c: HandleWaterTemp(tN2kMsg const&) at /home/tothg/own/projects/esp/hajo/main/devices/signalk/NMEA2000-SignalK-Gateway.cpp:215
0x40091f86: sendN2KMessageToSignalK(tN2kMsg const&) at /home/tothg/own/projects/esp/hajo/main/devices/signalk/NMEA2000-SignalK-Gateway.cpp:683
0x4008bec5: SignalkIncomingMessageHandler::HandleMsg(tN2kMsg const&) at /home/tothg/own/projects/esp/hajo/main/devices/signalk/hajo_signalk.cpp:96
0x40096e92: tNMEA2000::RunMessageHandlers(tN2kMsg const&) at /home/tothg/own/projects/esp/hajo/components/NMEA2000/src/NMEA2000.cpp:2685 (discriminator 8)
0x40099611: tNMEA2000::ParseMessages() at /home/tothg/own/projects/esp/hajo/components/NMEA2000/src/NMEA2000.cpp:2668
0x4008c3c5: task_main_poll(void*) at /home/tothg/own/projects/esp/hajo/main/n2k/n2k_receiver.cpp:80 (discriminator 1)
0x40033c6e: vPortTaskWrapper at /home/tothg/own/projects/esp-idf/components/freertos/FreeRTOS-Kernel/portable/xtensa/port.c:134
```