#pragma once

#include <stdint.h>
#include <string.h>

#include "pico/scanvideo.h"


#define FONT_WIDTH 8
#define FONT_HEIGHT 8
#define SCREEN_WIDTH_CHARS 40
#define SCREEN_HEIGHT_CHARS 40

#define FONT_HEIGHT5by5 5
#define FONT_WIDTH5by5 5

#define SCREEN_WIDTH 160
#define SCREEN_HEIGHT 120

extern int cur_x, cur_y;

#define FRAMEBUFFER_COUNT 2

#define VGA_RGB_555(r,g,b) ((r##UL<<0)|(g##UL<<6)|(b##UL << 11))

extern uint16_t pixel_framebuffer[FRAMEBUFFER_COUNT][SCREEN_WIDTH * SCREEN_HEIGHT];

extern int current_framebuffer;

extern uint16_t clear_colour;

extern uint16_t color_palette[16];

void swap_framebuffers();
void set_clear_colour(uint16_t colour);
void draw_pixel(int x, int y, uint16_t color);
void write_smallchar_to_framebuffer(int x, int y, char ch, uint8_t attribute);
void write_smallstring_to_framebuffer(const char* str, uint8_t attribute);
void write_char_to_framebuffer(int x, int y, char ch, uint8_t attribute);
void write_string_to_framebuffer(const char* str, uint8_t attribute);