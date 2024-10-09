#include "primitive.h"
#include "framebuffer.h"

#include "rasterrage.h"

static semaphore_t video_initted;
static semaphore_t drawing_semaphore;

// Timing variables
absolute_time_t last_time;
float frame_time = 0.0f;
float fps = 0.0f;

int main(void) {
    //set_sys_clock_khz(200000, true);
    stdio_init_all();
    // Initialize the pixel framebuffer with black pixels
    memset(pixel_framebuffer, 0, sizeof(pixel_framebuffer));

    // initialize video and interrupts on core 0
    scanvideo_setup(&vga_mode);
    scanvideo_timing_enable(true);
    sem_release(&video_initted);

    // create a semaphore to be posted when video init is complete
    sem_init(&video_initted, 0, 1);

    // launch all the video on core 1
    multicore_launch_core1(core1_func);
  
    while (true) {
        absolute_time_t start_time = get_absolute_time();
   
        cur_x = 0; cur_y = 0;
        write_string_to_framebuffer("BitBang VGA - RP2040\n", 0x0F);
        write_string_to_framebuffer("Some text\n", 0x0F);
        write_string_to_framebuffer("Some more text\n", 0x0F);
        write_string_to_framebuffer("Some more clipping\n", 0x0F);

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


        scanvideo_wait_for_vblank();   // Wait for vertical blanking
        swap_framebuffers(); 
    }
}
