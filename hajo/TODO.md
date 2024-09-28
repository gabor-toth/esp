Short term
==========

- gateway
    - pitch correction
- hűtő
    - ASR-03DA, ASR-02DD Szilárdtest relé https://www.hestore.hu/prod_10035237.html
    - relé 3v SMT https://www.hestore.hu/prod_10039590.html
    - SSR DIP8-ban https://www.hestore.hu/prod_10028866.html
    - IR530n https://electronics.stackexchange.com/questions/393066/how-to-control-12v-from-5v-using-transistor

Nyák
====

- kijelző
    - BC557 helyett MOSFET, hogy ne legyen feszültségesés?

Before go live
==============

- adc read on timer only
- round up/down when displaying data

Long term plans
===============

- configuration via ISO-11783 commands
- configuration on display
    - dimmer
    - off timer
- configuration rudder
    - direction, offset, R values
- configuration on gateway
    - correction for pitch

```
Display 1

n2k_sender I (78558) adc: Channel 4 uzemanyag_h Raw: 3986 Voltage:  433mV Display:   433 (corr 0)
Guru Meditation Error: Core  0 panic'ed (LoadProhibited). Exception was unhandled.

Core  0 register dump:
PC      : 0x4009b4f1  PS      : 0x00060130  A0      : 0x8009b5b5  A1      : 0x3ffdec20  
0x4009b4f1: get_prop_core at /home/tothg/own/projects/esp/hajo/components/lvgl/src/core/lv_obj_style.c:631

A2      : 0x3ffce15c  A3      : 0x00000008  A4      : 0x00000000  A5      : 0x00000000  
A6      : 0x3ffce0e8  A7      : 0x00000001  A8      : 0x3ffce0f0  A9      : 0x00000001  
A10     : 0x00000003  A11     : 0x000000cf  A12     : 0x000000d4  A13     : 0x00000000  
A14     : 0x3ffdee50  A15     : 0x00000000  SAR     : 0x0000001d  EXCCAUSE: 0x0000001c  
EXCVADDR: 0x0000000a  LBEG    : 0x000000d4  LEND    : 0x00000000  LCOUNT  : 0x40027820  
0x40027820: _xt_handle_exc at /home/tothg/own/projects/esp-idf/components/xtensa/xtensa_vectors.S:739



Backtrace: 0x4009b4ee:0x3ffdec20 0x4009b5b2:0x3ffdec70 0x400961a9:0x3ffdeca0 0x40096e95:0x3ffded50 0x4009392e:0x3ffded90 0x400a8789:0x3ffdedc0 0x4009392e:0x3ffdedf0 0x400939f5:0x3ffdee20 0x40093b06:0x3ffdee50 0x4009ceb1:0x3ffdee90 0x4009cf81:0x3ffdeed0 0x4009cf13:0x3ffdef20 0x4009cf81:0x3ffdef60 0x4009d20c:0x3ffdefb0 0x4009d426:0x3ffdefe0 0x4009d5c0:0x3ffdf060 0x4009d6f0:0x3ffdf090 0x4009d902:0x3ffdf0c0 0x400a66d1:0x3ffdf0f0 0x400a679f:0x3ffdf120 0x4008b15b:0x3ffdf150 0x4002f1ba:0x3ffdf210
0x4009b4ee: get_prop_core at /home/tothg/own/projects/esp/hajo/components/lvgl/src/core/lv_obj_style.c:631
0x4009b5b2: lv_obj_get_style_prop at /home/tothg/own/projects/esp/hajo/components/lvgl/src/core/lv_obj_style.c:229
0x400961a9: lv_obj_get_style_border_post at /home/tothg/own/projects/esp/hajo/components/lvgl/src/core/lv_obj_style_gen.h:267
 (inlined by) lv_obj_draw at /home/tothg/own/projects/esp/hajo/components/lvgl/src/core/lv_obj.c:529
0x40096e95: lv_obj_event at /home/tothg/own/projects/esp/hajo/components/lvgl/src/core/lv_obj.c:873
0x4009392e: lv_obj_event_base at /home/tothg/own/projects/esp/hajo/components/lvgl/src/core/lv_event.c:96
0x400a8789: lv_bar_event at /home/tothg/own/projects/esp/hajo/components/lvgl/src/widgets/lv_bar.c:515
0x4009392e: lv_obj_event_base at /home/tothg/own/projects/esp/hajo/components/lvgl/src/core/lv_event.c:96
0x400939f5: event_send_core at /home/tothg/own/projects/esp/hajo/components/lvgl/src/core/lv_event.c:452
0x40093b06: lv_event_send at /home/tothg/own/projects/esp/hajo/components/lvgl/src/core/lv_event.c:74
0x4009ceb1: lv_obj_redraw at /home/tothg/own/projects/esp/hajo/components/lvgl/src/core/lv_refr.c:148
0x4009cf81: refr_obj at /home/tothg/own/projects/esp/hajo/components/lvgl/src/core/lv_refr.c:970
0x4009cf13: lv_obj_redraw at /home/tothg/own/projects/esp/hajo/components/lvgl/src/core/lv_refr.c:182
0x4009cf81: refr_obj at /home/tothg/own/projects/esp/hajo/components/lvgl/src/core/lv_refr.c:970
0x4009d20c: refr_obj_and_children at /home/tothg/own/projects/esp/hajo/components/lvgl/src/core/lv_refr.c:854
0x4009d426: refr_area_part at /home/tothg/own/projects/esp/hajo/components/lvgl/src/core/lv_refr.c:789
0x4009d5c0: refr_area at /home/tothg/own/projects/esp/hajo/components/lvgl/src/core/lv_refr.c:678
0x4009d6f0: refr_invalid_areas at /home/tothg/own/projects/esp/hajo/components/lvgl/src/core/lv_refr.c:616
0x4009d902: _lv_disp_refr_timer at /home/tothg/own/projects/esp/hajo/components/lvgl/src/core/lv_refr.c:324
0x400a66d1: lv_timer_exec at /home/tothg/own/projects/esp/hajo/components/lvgl/src/misc/lv_timer.c:313 (discriminator 2)
0x400a679f: lv_timer_handler at /home/tothg/own/projects/esp/hajo/components/lvgl/src/misc/lv_timer.c:109
0x4008b15b: lv_task_handler at /home/tothg/own/projects/esp/hajo/components/lvgl/src/lv_api_map.h:37
 (inlined by) guiTask at /home/tothg/own/projects/esp/hajo/main/devices/display/display_main.c:192
0x4002f1ba: vPortTaskWrapper at /home/tothg/own/projects/esp-idf/components/freertos/FreeRTOS-Kernel/portable/xtensa/port.c:134




```

```
Display 2

n2k_sender I (81058) adc: Channel 4 uzemanyag_h Raw: 3988 Voltage:  433mV Display:   433 (corr 0)
E (88318) task_wdt: Task watchdog got triggered. The following tasks/users did not reset the watchdog in time:
E (88318) task_wdt:  - IDLE (CPU 0)
E (88318) task_wdt: Tasks currently running:
E (88318) task_wdt: CPU 0: gui
E (88318) task_wdt: Aborting.
E (88318) task_wdt: Print CPU 0 (current core) backtrace




Backtrace: 0x400a73bb:0x3ffdeb50 0x400a7426:0x3ffdeb80 0x400a75de:0x3ffdebb0 0x400a391e:0x3ffdebe0 0x400b462b:0x3ffdec10 0x400b498d:0x3ffdec50 0x400afc79:0x3ffdec80 0x400b09a3:0x3ffded30 0x400a03c9:0x3ffded60 0x40096271:0x3ffded90 0x40096e95:0x3ffdee40 0x4009392e:0x3ffdee80 0x400939f5:0x3ffdeeb0 0x40093b06:0x3ffdeee0 0x4009ceb1:0x3ffdef20 0x4009cf81:0x3ffdef60 0x4009d20c:0x3ffdefb0 0x4009d426:0x3ffdefe0 0x4009d5c0:0x3ffdf060 0x4009d6f0:0x3ffdf090 0x4009d902:0x3ffdf0c0 0x400a66d1:0x3ffdf0f0 0x400a679f:0x3ffdf120 0x4008b15b:0x3ffdf150 0x4002f1ba:0x3ffdf210
0x400a73bb: search_suitable_block at /home/tothg/own/projects/esp/hajo/components/lvgl/src/misc/lv_tlsf.c:574
0x400a7426: block_locate_free at /home/tothg/own/projects/esp/hajo/components/lvgl/src/misc/lv_tlsf.c:769
0x400a75de: lv_tlsf_malloc at /home/tothg/own/projects/esp/hajo/components/lvgl/src/misc/lv_tlsf.c:1101
0x400a391e: lv_mem_alloc at /home/tothg/own/projects/esp/hajo/components/lvgl/src/misc/lv_mem.c:134
0x400b462b: allocate_item at /home/tothg/own/projects/esp/hajo/components/lvgl/src/draw/sw/lv_draw_sw_gradient.c:191
0x400b498d: lv_gradient_get at /home/tothg/own/projects/esp/hajo/components/lvgl/src/draw/sw/lv_draw_sw_gradient.c:271
0x400afc79: draw_bg at /home/tothg/own/projects/esp/hajo/components/lvgl/src/draw/sw/lv_draw_sw_rect.c:167
0x400b09a3: lv_draw_sw_rect at /home/tothg/own/projects/esp/hajo/components/lvgl/src/draw/sw/lv_draw_sw_rect.c:73
0x400a03c9: lv_draw_rect at /home/tothg/own/projects/esp/hajo/components/lvgl/src/draw/lv_draw_rect.c:66
0x40096271: lv_obj_draw at /home/tothg/own/projects/esp/hajo/components/lvgl/src/core/lv_obj.c:561
0x40096e95: lv_obj_event at /home/tothg/own/projects/esp/hajo/components/lvgl/src/core/lv_obj.c:873
0x4009392e: lv_obj_event_base at /home/tothg/own/projects/esp/hajo/components/lvgl/src/core/lv_event.c:96
0x400939f5: event_send_core at /home/tothg/own/projects/esp/hajo/components/lvgl/src/core/lv_event.c:452
0x40093b06: lv_event_send at /home/tothg/own/projects/esp/hajo/components/lvgl/src/core/lv_event.c:74
0x4009ceb1: lv_obj_redraw at /home/tothg/own/projects/esp/hajo/components/lvgl/src/core/lv_refr.c:148
0x4009cf81: refr_obj at /home/tothg/own/projects/esp/hajo/components/lvgl/src/core/lv_refr.c:970
0x4009d20c: refr_obj_and_children at /home/tothg/own/projects/esp/hajo/components/lvgl/src/core/lv_refr.c:854
0x4009d426: refr_area_part at /home/tothg/own/projects/esp/hajo/components/lvgl/src/core/lv_refr.c:789
0x4009d5c0: refr_area at /home/tothg/own/projects/esp/hajo/components/lvgl/src/core/lv_refr.c:678
0x4009d6f0: refr_invalid_areas at /home/tothg/own/projects/esp/hajo/components/lvgl/src/core/lv_refr.c:616
0x4009d902: _lv_disp_refr_timer at /home/tothg/own/projects/esp/hajo/components/lvgl/src/core/lv_refr.c:324
0x400a66d1: lv_timer_exec at /home/tothg/own/projects/esp/hajo/components/lvgl/src/misc/lv_timer.c:313 (discriminator 2)
0x400a679f: lv_timer_handler at /home/tothg/own/projects/esp/hajo/components/lvgl/src/misc/lv_timer.c:109
0x4008b15b: lv_task_handler at /home/tothg/own/projects/esp/hajo/components/lvgl/src/lv_api_map.h:37
 (inlined by) guiTask at /home/tothg/own/projects/esp/hajo/main/devices/display/display_main.c:192
0x4002f1ba: vPortTaskWrapper at /home/tothg/own/projects/esp-idf/components/freertos/FreeRTOS-Kernel/portable/xtensa/port.c:134

```

