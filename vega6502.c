/*
 * Copyright (c) 2021 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <string.h>

#include "pico.h"
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/scanvideo.h"
#include "pico/scanvideo/composable_scanline.h"
#include "pico/sync.h"

#include "font.h"

#define vga_mode vga_mode_320x240_60

void core1_func();

#define FONT_WIDTH 8
#define FONT_HEIGHT 8
#define SCREEN_WIDTH_CHARS 40
#define SCREEN_HEIGHT_CHARS 40

int cur_x = 0, cur_y = 0;

typedef struct {
    uint8_t ch;
    uint8_t at;  // High 4 bits: Foreground, Low 4 bits: Background
} TextCell;

// The framebuffer array
TextCell framebuffer[SCREEN_HEIGHT_CHARS * SCREEN_WIDTH_CHARS];

#define VGA_RGB_555(r,g,b) ((r##UL<<0)|(g##UL<<6)|(b##UL << 11))

#define X2(a) (a | (a << 16))

static const uint32_t white_pixel = X2(VGA_RGB_555(31, 31, 31));
static const uint32_t black_pixel = X2(VGA_RGB_555(0, 0, 0));

// Simple color bar program, which draws 7 colored bars: red, green, yellow, blow, magenta, cyan, white
// Can be used to check resister DAC correctness.
//
// Note this program also demonstrates running video on core 1, leaving core 0 free. It supports
// user input over USB or UART stdin, although all it does with it is invert the colors when you press SPACE

static semaphore_t video_initted;

#define COUNT 80

int num_token = 2 + 320 + 1 + 1;
#define MIN_RUN 3

uint16_t color_palette[16] = {
    VGA_RGB_555(0, 0, 0),    // 0: Black
    VGA_RGB_555(31, 0, 0),   // 1: Red
    VGA_RGB_555(0, 31, 0),   // 2: Green
    VGA_RGB_555(31, 31, 0),  // 3: Yellow
    VGA_RGB_555(0, 0, 31),   // 4: Blue
    VGA_RGB_555(31, 0, 31),  // 5: Magenta
    VGA_RGB_555(0, 31, 31),  // 6: Cyan
    VGA_RGB_555(31, 31, 31), // 7: White
    VGA_RGB_555(15, 15, 15), // 8: Dark Gray
    VGA_RGB_555(31, 15, 15), // 9: Light Red
    VGA_RGB_555(15, 31, 15), // A: Light Green
    VGA_RGB_555(31, 31, 15), // B: Light Yellow
    VGA_RGB_555(15, 15, 31), // C: Light Blue
    VGA_RGB_555(31, 15, 31), // D: Light Magenta
    VGA_RGB_555(15, 31, 31), // E: Light Cyan
    VGA_RGB_555(25, 25, 25)  // F: Bright White
};
/*uint16_t precomputed_font_pixels[128][8][8];

void precompute_font_pixels(uint8_t fg_color, uint8_t bg_color, uint16_t pixels[8][8]) {
    for (int row = 0; row < 8; ++row) {
        uint8_t row_bits = font_data[ch][row];
        for (int bit = 0; bit < 8; ++bit) {
            if (row_bits & (1 << (7 - bit))) {
                pixels[row][bit] = color_palette[fg_color]; // Foreground color
            } else {
                pixels[row][bit] = color_palette[bg_color]; // Background color
            }
        }
    }
}*/


int main(void) {
    stdio_init_all();

    set_sys_clock_khz (150000, true);

    memset(framebuffer, 0, sizeof(uint8_t) * SCREEN_HEIGHT_CHARS * SCREEN_WIDTH_CHARS);

    write_string_to_framebuffer("BitBang'd VGA - RP2040 150MHz Overclock\n", 0x0F);
    write_string_to_framebuffer("Running at 320x240 60Hz\n", 0x0F);
    write_string_to_framebuffer("40x40 text mode - 16 colours\n", 0x0F);
    
// Line 1: Display all colors with matching foreground and background
    for (uint8_t color = 0; color <= 0xF; color++) {
        char buffer[SCREEN_WIDTH_CHARS + 1]; // Buffer to hold the line of text

        // Fill the rest of the line with the same color
        for (int i = strlen(buffer); i < SCREEN_WIDTH_CHARS; i++) {
            buffer[i] = ' ';
        }
        buffer[SCREEN_WIDTH_CHARS] = '\0'; // Null-terminate the string

        write_string_to_framebuffer(buffer, (color << 4) | color);
    }



    // Showcase all possible foreground and background combinations in individual cells
    for (uint8_t fg = 0; fg <= 0xF; fg++) {
        for (uint8_t bg = 0; bg <= 0xF; bg++) {
            uint8_t attribute = (fg << 4) | bg;
            write_to_framebuffer(cur_x, cur_y, 'A', attribute);

            // Move to the next cell
            cur_x++;
            
            if (bg == 0xF) cur_x += 8; // Skip one column for spacing

            if (cur_x >= SCREEN_WIDTH_CHARS) {
                cur_x = 0;
                cur_y++;
                if (cur_y >= SCREEN_HEIGHT_CHARS) break; // Stop if we exceed screen height
            }
        }
    }

    // initialize video and interrupts on core 0
    scanvideo_setup(&vga_mode);
    scanvideo_timing_enable(true);
    sem_release(&video_initted);

    // create a semaphore to be posted when video init is complete
    sem_init(&video_initted, 0, 1);

    // launch all the video on core 1, so it isn't affected by USB handling on core 0
    multicore_launch_core1(core1_func);

    // wait for initialization of video to be complete
    sem_acquire_blocking(&video_initted);

    while (true) {
        // prevent tearing when we invert - if you're astute you'll notice this actually causes
        // a fixed tear a number of scanlines from the top. this is caused by pre-buffering of scanlines
        // and is too detailed a topic to fix here.
        scanvideo_wait_for_vblank();
    }
}

void __time_critical_func(draw_pixels)(scanvideo_scanline_buffer_t *buffer) {
    int scanline = scanvideo_scanline_number(buffer->scanline_id);
    int char_row = scanline / FONT_HEIGHT;
    int row_pixel_offset = scanline % FONT_HEIGHT;

    uint32_t *buf = buffer->data;
    uint16_t *p = (uint16_t *)(buf + 2);

    TextCell *cell = &framebuffer[char_row * SCREEN_WIDTH_CHARS];
    char *font_row;
    uint16_t fg_pixel, bg_pixel;

    // Precompute the first character's pixel row and colors
    font_row = font_data[cell->ch];
    fg_pixel = color_palette[cell->at & 0x0F];
    bg_pixel = color_palette[(cell->at >> 4) & 0x0F];

    uint8_t row = font_row[row_pixel_offset];

    // Precompute the first two pixels to avoid unnecessary checks in the loop
    uint16_t pixel1 = (row & 0x80) ? fg_pixel : bg_pixel;
    uint16_t pixel2 = (row & 0x40) ? fg_pixel : bg_pixel;

    buf[0] = COMPOSABLE_RAW_RUN | (pixel1 << 16);
    buf[1] = (num_token - 3) - MIN_RUN | (pixel2 << 16);

    // Unrolled loop for the remaining 6 pixels of the first character
    *p++ = (row & 0x20) ? fg_pixel : bg_pixel;
    *p++ = (row & 0x10) ? fg_pixel : bg_pixel;
    *p++ = (row & 0x08) ? fg_pixel : bg_pixel;
    *p++ = (row & 0x04) ? fg_pixel : bg_pixel;
    *p++ = (row & 0x02) ? fg_pixel : bg_pixel;
    *p++ = (row & 0x01) ? fg_pixel : bg_pixel;

    // Process remaining characters in the row
    for (int x = 1; x < SCREEN_WIDTH_CHARS; x++) {
        cell++;
        font_row = font_data[cell->ch];
        fg_pixel = color_palette[cell->at & 0x0F];
        bg_pixel = color_palette[(cell->at >> 4) & 0x0F];
        row = font_row[row_pixel_offset];

        // Unrolled loop for all 8 pixels of the current character
        *p++ = (row & 0x80) ? fg_pixel : bg_pixel;
        *p++ = (row & 0x40) ? fg_pixel : bg_pixel;
        *p++ = (row & 0x20) ? fg_pixel : bg_pixel;
        *p++ = (row & 0x10) ? fg_pixel : bg_pixel;
        *p++ = (row & 0x08) ? fg_pixel : bg_pixel;
        *p++ = (row & 0x04) ? fg_pixel : bg_pixel;
        *p++ = (row & 0x02) ? fg_pixel : bg_pixel;
        *p++ = (row & 0x01) ? fg_pixel : bg_pixel;
    }

    // Handle the end of the line
    *p++ = 0; // Last pixel must be black
    *p++ = (num_token % 2) ? COMPOSABLE_EOL_ALIGN : COMPOSABLE_EOL_SKIP_ALIGN;

    buffer->data_used = (num_token + 1) / 2;
    buffer->status = SCANLINE_OK;
}

void core1_func() {

    while (true) {
        scanvideo_scanline_buffer_t *scanline_buffer = scanvideo_begin_scanline_generation(true);
        draw_pixels(scanline_buffer);
        scanvideo_end_scanline_generation(scanline_buffer);
    }
}

void write_to_framebuffer(int x, int y, char ch, uint8_t attribute) {
    if (x >= 0 && x < SCREEN_WIDTH_CHARS && y >= 0 && y < SCREEN_HEIGHT_CHARS) {
        framebuffer[x + (y * SCREEN_WIDTH_CHARS)].ch = ch;
        framebuffer[x + (y * SCREEN_WIDTH_CHARS)].at = attribute;
    }
}

void write_string_to_framebuffer(const char* str, uint8_t attribute) {
    while (*str) {
        if (*str == '\n') {
            // Handle newline character
            cur_x = 0;
            cur_y++;
        } else {
            // Calculate the index in the linear framebuffer
            int index = cur_x + (cur_y * SCREEN_WIDTH_CHARS);

            // Write character and attribute directly to the framebuffer
            framebuffer[index].ch = *str;
            framebuffer[index].at = attribute; // Uncomment if using attributes

            // Move the cursor to the next position
            cur_x++;

            // If the cursor reaches the end of the line, move to the next line
            if (cur_x >= SCREEN_WIDTH_CHARS) {
                cur_x = 0;
                cur_y++;
            }
        }

        // If the cursor reaches the bottom of the screen, wrap around
        if (cur_y >= SCREEN_HEIGHT_CHARS) {
            cur_y = 0;
        }

        str++;
    }
}
