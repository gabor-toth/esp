#ifndef HAJO_ENGINE_DISPLAY_DRAW_H
#define HAJO_ENGINE_DISPLAY_DRAW_H

extern void engine_display_setup_display();

extern void engine_display_draw_screen(const char *cause);

extern void engine_display_draw_logo();

extern void engine_display_onoff(bool on);

extern bool engine_display_is_on;

#endif //HAJO_ENGINE_DISPLAY_DRAW_H
