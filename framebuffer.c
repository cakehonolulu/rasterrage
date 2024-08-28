#include "framebuffer.h"

#include "font.h"
#include "small_font.h"
#include "tiny_font.h"

uint16_t pixel_framebuffer[FRAMEBUFFER_COUNT][SCREEN_WIDTH * SCREEN_HEIGHT];

int current_framebuffer = 0; // Index of the current front buffer

int cur_x = 0, cur_y = 0;

uint16_t clear_colour = PICO_SCANVIDEO_PIXEL_FROM_RGB5(0, 0, 0);

uint16_t color_palette[16] = {
    PICO_SCANVIDEO_PIXEL_FROM_RGB5(0, 0, 0),    // 0: Black
    PICO_SCANVIDEO_PIXEL_FROM_RGB5(31, 0, 0),   // 1: Red
    PICO_SCANVIDEO_PIXEL_FROM_RGB5(0, 31, 0),   // 2: Green
    PICO_SCANVIDEO_PIXEL_FROM_RGB5(31, 31, 0),  // 3: Yellow
    PICO_SCANVIDEO_PIXEL_FROM_RGB5(0, 0, 31),   // 4: Blue
    PICO_SCANVIDEO_PIXEL_FROM_RGB5(31, 0, 31),  // 5: Magenta
    PICO_SCANVIDEO_PIXEL_FROM_RGB5(0, 31, 31),  // 6: Cyan
    PICO_SCANVIDEO_PIXEL_FROM_RGB5(31, 31, 31), // 7: White
    PICO_SCANVIDEO_PIXEL_FROM_RGB5(15, 15, 15), // 8: Dark Gray
    PICO_SCANVIDEO_PIXEL_FROM_RGB5(31, 15, 15), // 9: Light Red
    PICO_SCANVIDEO_PIXEL_FROM_RGB5(15, 31, 15), // A: Light Green
    PICO_SCANVIDEO_PIXEL_FROM_RGB5(31, 31, 15), // B: Light Yellow
    PICO_SCANVIDEO_PIXEL_FROM_RGB5(15, 15, 31), // C: Light Blue
    PICO_SCANVIDEO_PIXEL_FROM_RGB5(31, 15, 31), // D: Light Magenta
    PICO_SCANVIDEO_PIXEL_FROM_RGB5(15, 31, 31), // E: Light Cyan
    PICO_SCANVIDEO_PIXEL_FROM_RGB5(25, 25, 25)  // F: Bright White
};


void swap_framebuffers() {
    // Copy the current framebuffer to the next framebuffer
    memcpy(pixel_framebuffer[1], pixel_framebuffer[0], sizeof(uint16_t) * SCREEN_WIDTH * SCREEN_HEIGHT);

    memset(pixel_framebuffer[0], clear_colour, sizeof(uint16_t) * SCREEN_WIDTH * SCREEN_HEIGHT);
}

void set_clear_colour(uint16_t colour)
{
    clear_colour = colour;
}

void draw_pixel(int x, int y, uint16_t color) {
    if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT) {
        pixel_framebuffer[current_framebuffer][y * SCREEN_WIDTH + x] = color;
    }
}


void write_smallchar_to_framebuffer(int x, int y, char ch, uint8_t attribute) {
    if (x >= 0 && x < SCREEN_WIDTH_CHARS && y >= 0 && y < SCREEN_HEIGHT_CHARS) {
        uint16_t fg_color = color_palette[attribute & 0x0F];
        uint16_t bg_color = color_palette[(attribute >> 4) & 0x0F];

        for (int row = 0; row < FONT_HEIGHT5by5; row++) {
            uint8_t row_data = small_font_data[(uint8_t)ch][row];
            row_data = row_data << 3;
            for (int col = 0; col < FONT_WIDTH5by5; col++) {
                uint16_t color = (row_data & (1 << (7 - col))) ? fg_color : bg_color;
                draw_pixel(x * FONT_WIDTH5by5 + col, y * FONT_HEIGHT5by5 + row, color);
            }
        }
    }
}


void write_smallstring_to_framebuffer(const char* str, uint8_t attribute) {
    while (*str) {
        if (*str == '\n') {
            cur_x = 0;
            cur_y++;
        } else {
            write_smallchar_to_framebuffer(cur_x, cur_y, *str, attribute);
            cur_x++;
            if (cur_x >= SCREEN_WIDTH_CHARS) {
                cur_x = 0;
                cur_y++;
            }
        }

        if (cur_y >= SCREEN_HEIGHT_CHARS) {
            cur_y = 0;
        }

        str++;
    }
}
void write_char_to_framebuffer(int x, int y, char ch, uint8_t attribute) {
    if (x >= 0 && x < SCREEN_WIDTH_CHARS && y >= 0 && y < SCREEN_HEIGHT_CHARS) {
        uint16_t fg_color = color_palette[attribute & 0x0F];
        uint16_t bg_color = color_palette[(attribute >> 4) & 0x0F];

        for (int row = 0; row < FONT_HEIGHT; row++) {
            uint8_t row_data = font_data[(uint8_t)ch][row];
            for (int col = 0; col < FONT_WIDTH; col++) {
                uint16_t color = (row_data & (1 << (7 - col))) ? fg_color : bg_color;
                draw_pixel(x * FONT_WIDTH + col, y * FONT_HEIGHT + row, color);
            }
        }
    }
}

void write_string_to_framebuffer(const char* str, uint8_t attribute) {
    while (*str) {
        if (*str == '\n') {
            cur_x = 0;
            cur_y++;
        } else {
            if (*str != ' ') write_char_to_framebuffer(cur_x, cur_y, *str, attribute);
            cur_x++;
            if (cur_x >= SCREEN_WIDTH_CHARS) {
                cur_x = 0;
                cur_y++;
            }
        }

        if (cur_y >= SCREEN_HEIGHT_CHARS) {
            cur_y = 0;
        }

        str++;
    }
}
