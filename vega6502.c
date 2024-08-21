#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include "pico.h"
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/scanvideo.h"
#include "pico/scanvideo/composable_scanline.h"
#include "pico/sync.h"
#include "font.h"

#if 0
#define vga_mode vga_mode_160x120_60
#else
#define vga_mode vga_mode_320x240_60
#endif

#define FONT_WIDTH 8
#define FONT_HEIGHT 8
#define SCREEN_WIDTH_CHARS 40
#define SCREEN_HEIGHT_CHARS 40

#if 0
#define SCREEN_WIDTH 160
#define SCREEN_HEIGHT 120
#else
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#endif

int cur_x = 0, cur_y = 0;

#if 0
#define FRAMEBUFFER_COUNT 2
uint16_t pixel_framebuffer[FRAMEBUFFER_COUNT][SCREEN_WIDTH * SCREEN_HEIGHT];
int current_framebuffer = 0; // Index of the current front buffer
#else
// Linear pixel framebuffer
uint16_t pixel_framebuffer[SCREEN_WIDTH * SCREEN_HEIGHT];
#endif

static semaphore_t video_initted;

#define VGA_RGB_555(r,g,b) ((r##UL<<0)|(g##UL<<6)|(b##UL << 11))

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

void draw_pixel(int x, int y, uint16_t color) {
    if (x >= 0 && x < (SCREEN_WIDTH - 1) && y >= 0 && y < SCREEN_HEIGHT) {
#if 0
        pixel_framebuffer[0][y * SCREEN_WIDTH + x] = color;
#else
        pixel_framebuffer[y * SCREEN_WIDTH + x] = color;
#endif
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
            write_char_to_framebuffer(cur_x, cur_y, *str, attribute);
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

void draw_line(int x0, int y0, int x1, int y1, uint16_t color) {
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    while (true) {
        draw_pixel(x0, y0, color);  // Draw the current pixel

        if (x0 == x1 && y0 == y1) break;  // If we've reached the end of the line, stop

        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void draw_triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t color) {
    // Draw lines between each pair of vertices
    draw_line(x0, y0, x1, y1, color);
    draw_line(x1, y1, x2, y2, color);
    draw_line(x2, y2, x0, y0, color);
}

void swap(int *a, int *b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

void draw_filled_triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t color) {
    // Sort vertices by y-coordinate (y0 <= y1 <= y2)
    if (y0 > y1) { swap(&x0, &x1); swap(&y0, &y1); }
    if (y0 > y2) { swap(&x0, &x2); swap(&y0, &y2); }
    if (y1 > y2) { swap(&x1, &x2); swap(&y1, &y2); }

    // Handle the special case where all three vertices are on the same line (flat triangle)
    if (y0 == y2) {
        int start = x0, end = x0;
        if (x1 < start) start = x1;
        if (x2 < start) start = x2;
        if (x1 > end) end = x1;
        if (x2 > end) end = x2;
        for (int x = start; x <= end; x++) {
            draw_pixel(x, y0, color);
        }
        return;
    }

    int total_height = y2 - y0;

    for (int i = 0; i < total_height; i++) {
        bool second_half = i > y1 - y0 || y1 == y0;
        int segment_height = second_half ? y2 - y1 : y1 - y0;

        float alpha = (float)i / total_height;
        float beta = (float)(i - (second_half ? y1 - y0 : 0)) / segment_height;

        int A = x0 + (x2 - x0) * alpha;
        int B = second_half ? x1 + (x2 - x1) * beta : x0 + (x1 - x0) * beta;

        if (A > B) swap(&A, &B);

        for (int x = A; x <= B; x++) {
            draw_pixel(x, y0 + i, color);
        }
    }
}

void draw_rectangle(int x0, int y0, int x1, int y1, uint16_t color) {
    for (int x = x0; x <= x1; x++) {
        draw_pixel(x, y0, color); // Top edge
        draw_pixel(x, y1, color); // Bottom edge
    }
    for (int y = y0; y <= y1; y++) {
        draw_pixel(x0, y, color); // Left edge
        draw_pixel(x1, y, color); // Right edge
    }
}

void draw_square(int x, int y, int size, uint16_t color) {
    draw_rectangle(x, y, x + size - 1, y + size - 1, color);
}

void draw_filled_rectangle(int x0, int y0, int x1, int y1, uint16_t color) {
    for (int y = y0; y <= y1; y++) {
        for (int x = x0; x <= x1; x++) {
            draw_pixel(x, y, color);
        }
    }
}

void draw_filled_square(int x, int y, int size, uint16_t color) {
    draw_filled_rectangle(x, y, x + size - 1, y + size - 1, color);
}

void __time_critical_func(draw_pixels)(scanvideo_scanline_buffer_t *buffer) {
    int scanline = scanvideo_scanline_number(buffer->scanline_id);
    uint16_t *p = (uint16_t *)(buffer->data + 2);  // Start at the third word of the buffer

#if 0
    // Calculate the starting pixel position in the pixel_framebuffer
    uint16_t *pixel_ptr = &pixel_framebuffer[0][scanline * SCREEN_WIDTH];
#else
    // Calculate the starting pixel position in the pixel_framebuffer
    uint16_t *pixel_ptr = &pixel_framebuffer[scanline * SCREEN_WIDTH];
#endif

    // Precompute the first two pixels
    uint16_t pixel1 = *pixel_ptr++;
    uint16_t pixel2 = *pixel_ptr++;

    // Insert them into the buffer using the appropriate VGA format
    buffer->data[0] = COMPOSABLE_RAW_RUN | (pixel1 << 16);
    buffer->data[1] = (SCREEN_WIDTH - 3) | (pixel2 << 16);

    // Now handle the remaining pixels for this scanline
    for (int x = 2; x < SCREEN_WIDTH; x++) {
        *p++ = *pixel_ptr++;
    }

    // Handle the end of the line
    *p++ = 0; // Last pixel must be black
    *p++ = COMPOSABLE_EOL_SKIP_ALIGN;

    buffer->data_used = (p - (uint16_t *)(buffer->data + 2)) + 2;
    buffer->status = SCANLINE_OK;
}

void core1_func() {
    float rotation_speed = 0.02f; // Adjust this value to change the rotation speed
    while (true) {
        scanvideo_scanline_buffer_t *scanline_buffer = scanvideo_begin_scanline_generation(true);
        draw_pixels(scanline_buffer);
        scanvideo_end_scanline_generation(scanline_buffer);
    }
}

#if 0
void swap_framebuffers() {
    // Copy the current framebuffer to the next framebuffer
    memcpy(pixel_framebuffer[1], pixel_framebuffer[0], sizeof(uint16_t) * SCREEN_WIDTH * SCREEN_HEIGHT);

    memset(pixel_framebuffer[0], 0, sizeof(uint16_t) * SCREEN_WIDTH * SCREEN_HEIGHT);
}
#endif

int main(void) {
    stdio_init_all();

    // Initialize the pixel framebuffer with black pixels
    memset(pixel_framebuffer, 0, sizeof(pixel_framebuffer));

#if 1
    write_string_to_framebuffer("BitBang'd VGA - RP2040 Base Clock\n", 0x0F);
    write_string_to_framebuffer("Running at 320x240 60Hz\n", 0x0F);
    write_string_to_framebuffer("40x40 text mode - 16 colours\n", 0x0F);

    // Draw a white line from top-left to bottom-right corner
    draw_line(0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT, color_palette[0x01]); // Red

    // Draw a red line from top-right to bottom-left corner
    draw_line(SCREEN_WIDTH - 1, 0, 0, SCREEN_HEIGHT, color_palette[0x0F]); // White

    draw_filled_triangle(50, 50, 150, 50, 100, 150, color_palette[0x02]); // White triangle

    draw_triangle(52, 52, 152, 52, 102, 152, color_palette[0x04]); // White triangle

    // Draw a white rectangle
    draw_rectangle(200, 50, 280, 100, color_palette[0x0F]); // White rectangle

    // Draw a red square
    draw_square(200, 120, 40, color_palette[0x01]); // Red square

    // Draw a filled blue rectangle
    draw_filled_rectangle(50, 180, 130, 230, color_palette[0x04]); // Blue filled rectangle

    // Draw a filled green square
    draw_filled_square(200, 180, 40, color_palette[0x02]); // Green filled square
#endif

    // initialize video and interrupts on core 0
    scanvideo_setup(&vga_mode);
    scanvideo_timing_enable(true);
    sem_release(&video_initted);

    // create a semaphore to be posted when video init is complete
    sem_init(&video_initted, 0, 1);

    // launch all the video on core 1
    multicore_launch_core1(core1_func);

    while (true) {
        scanvideo_wait_for_vblank();
#if 0
        swap_framebuffers();
#endif
    }
}
