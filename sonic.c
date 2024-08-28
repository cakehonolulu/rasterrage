#include "primitive.h"
#include "framebuffer.h"

#include "vega6502.h"

static semaphore_t video_initted;
static semaphore_t drawing_semaphore;

// Timing variables
absolute_time_t last_time;
float frame_time = 0.0f;
float fps = 0.0f;

static const char *sega_xpm[] = {
        "96 31 3 1",
        "     .................    ..................    ..................           ....       ......  ",
        "   ..++++++++++++++++.  ..+++++++++++++++++.  ..+++++++++++++++++.         ..++++..    .++++++. ",
        "  .++++++++++++++++++. .+++++++++++++++++++. .+++++++++++++++++++.        .++++++++.  .++....++.",
        " .+++++++++++++++++++..++++++++++++++++++++..++++++++++++++++++++.       .++++++++++...+.++++.+.",
        " .+++++++++++++++++++..++++++++++++++++++++..++++++++++++++++++++.      .++++++++++++..+.+..+.+.",
        ".++++++...............+++++++...............+++++++...............      .++++++++++++..+.++++.+.",
        ".+++++.++++++++++++++.++++++.++++++++++++++.+++++..++++++++++++++.     .++++++..++++++.+.+.+..+.",
        ".++++.+++++++++++++++.+++++.+++++++++++++++.++++.++++++++++++++++.     .++++++..++++++.+.+..+.+.",
        ".++++.+++++++++++++++.+++++.+++++++++++++++.++++.++++++++++++++++.     .+++++.++.+++++.++....++.",
        ".++++.+++++++++++++++.+++++.+++++++++++++++.++++.++++++++++++++++.    .++++++.++.++++++..+++++. ",
        ".++++.++++............+++++.+++.............++++.++++.............    .++++++.++.++++++.......  ",
        ".++++.++++++++++......+++++.+++++++++++++...++++.++++.+++++++++++.    .+++++.++++.+++++.        ",
        ".++++.++++++++++++....+++++.+++++++++++++...++++.++++.+++++++++++.   .++++++.++++.++++++.       ",
        ".++++.+++++++++++++...+++++.+++++++++++++...++++.++++.+++++++++++.   .++++++.++++.++++++.       ",
        ".+++++.+++++++++++++..+++++.+++++++++++++...++++.++++.+++++++++++.   .+++++.++++++.+++++.       ",
        " .+++++........+++++..+++++.................++++.++++........++++.  .++++++.++++++.++++++.      ",
        " .+++++++++++++.+++++.+++++.+++++++++++++...++++.++++.++++++.++++.  .++++++.++++++.++++++.      ",
        "  .+++++++++++++.++++.+++++.+++++++++++++...++++.++++.++++++.++++.  .+++++.++++++++.+++++.      ",
        "  .+++++++++++++.++++.+++++.+++++++++++++...++++.++++.++++++.++++. .++++++.++++++++.++++++.     ",
        "   ...++++++++++.++++.+++++.+++++++++++++...++++.++++.++++++.++++. .++++++.+++..+++.++++++.     ",
        "............++++.++++.+++++.+++.............++++.++++...++++.++++. .+++++.++++..++++.+++++.     ",
        ".+++++++++++++++.++++.+++++.+++++++++++++++.++++.+++++++++++.++++..++++++.++++++++++.++++++.    ",
        ".+++++++++++++++.++++.+++++.+++++++++++++++.++++.+++++++++++.++++..++++++.++++++++++.++++++.    ",
        ".+++++++++++++++.++++.+++++.+++++++++++++++.++++.+++++++++++.++++.++++++.++++++++++++.+++++.    ",
        ".++++++++++++++.+++++.++++++.++++++++++++++.+++++..+++++++++.++++.++++++.++++++++++++.++++++.   ",
        "...............++++++.+++++++................++++++..........+++++++++++.+++..........++++++.   ",
        ".+++++++++++++++++++. .++++++++++++++++++++..++++++++++++++++++++++++++.++++.+++++++++++++++.   ",
        ".+++++++++++++++++++. .++++++++++++++++++++..++++++++++++++++++++++++++.+++..++++++++++++++++.  ",
        ".++++++++++++++++++.   .+++++++++++++++++++. .+++++++++++++++++++++++++.+++..++++++++++++++++.  ",
        ".++++++++++++++++..     ..+++++++++++++++++.  ..+++++++++++++++++++++++.++...++++++++++++++++.  ",
        ".................         ..................    ..............................................  "};


static const char *sonic_xpm[] = {
        "               .         .....    .                    .         .....    .     ",
        "   ............+       ..+++.+....+        ............+       ..+++.+....+     ",
        "     ..+.@.+++.+         .+.@.+++.+          ..+.@.+++.+         .+.@.+++.+     ",
        "      ...#@.+++.         ...#@.+++.           ...#@.+++.         ...#@.+++.     ",
        "    ....@#@.+++.        ...@#@.+++.         ....@#@.+++.        ...@#@.+++.     ",
        "  ..+++.@#++$+++.      ..+.@#++$+++.      ..+++.@#++$+++.      ..+.@#++$+++.    ",
        " .+++++++++%$$++.     ..++++++%$$++.     .+++++++++%$$++.     ..++++++%$$++.    ",
        "  ...++++++%$$&+&    .++++++++%$$&+&      ...++++++%$$&+&    .++++++++%$$&+&    ",
        "   .....+++%$$&+&   .......+++%$$&+&       .....+++%$$&+&   .......+++%$$&+&    ",
        "    ....+++%$$&&&&     ....+++%$$&&&&       ....+++%$$&&&&     ....+++%$$&&&&   ",
        "     ....+@@$$&@&&      ....+@@$$&@&&        ....+@@$$&@&&      ....+@@$$&@&&   ",
        "    .+...@##@@@#&&     .+.#####@@@#&&       .+...@##@@@#&&     .+...@##@@@#&&   ",
        "   .+..%%@######@     .+#####&#####@       .+..%%@######@     .+..%%@######@    ",
        "   &&&%$$%@####@      &######&####@        &&&%$$%@####@      &###$$%@####@     ",
        "  &@@%$$$%..@@&#     &#####&&..@@&&       &@@%$$$%..@@&&     &####$$%..@@&&     ",
        "   &&%$$$%..&&###    @###&&$%..&&%%&       &&%$$$%..&&@@&    #####$$%..&&%%&    ",
        "  &&  %%%..@@&%#@@   @@@&%%%..@@&%%&          %%%..@@&@@@&   ####%%%..@@&%%&    ",
        " &##& ....@##@%@@&  @@@@ ....@##@%&       &&  ....@##@%@#&  #### ....@##@%&     ",
        "&##%%  ..@###@&@&&  @@@   ..@###@&       &@@&  ..@###@&@#&  ###   ..@###@&      ",
        "&##$+  ..@###@ @&&  @@    ..@###@   &    &@@&  ..@###@ @#&  ###   ..@###@   @   ",
        "&##$.....@##@+@@&&  @@  ..&.@##@    &   &@@@%.&..@##@  @#&  ##   ...@##@    @   ",
        "&##%$++...@@..@&&&  @@ .&& ..@@     &   &@@@%.....+++ @##&  ##   .&..@@     @   ",
        "&### .. ....@@@&&   @@     ....   & &   &@@@@.. .....+@##&  ## ..& .+..   @ @   ",
        "&###        &@&&&    @        ..  &&&   &@@@@&&     ..$##&  ##     .+     @@@   ",
        " &##        &&&&     @     &@ .. @&&    &@@@&       $$##&    #    &&&     @@    ",
        " &###        &&&      @   &@@.. @@&&     &@@&      &####&     #  &###   @@#@    ",
        " &###@       &&&          &@@@#@@&&      &@@@&     &###&        &######$##@&    ",
        "  &#@@@                  &@@@#@@@&        &@@&      &#&         &#####$#@@&     ",
        "   &&@@@                 &@@@@@&&          &@@&&     &           &###%@@&&      ",
        "       @@@@               &&&&&             &&&&&&&&              &&&&&&        "};


#define COLOR_WHITE PICO_SCANVIDEO_PIXEL_FROM_RGB5(255, 255, 255)
#define COLOR_BLUE  PICO_SCANVIDEO_PIXEL_FROM_RGB5(0, 0, 255)
#define COLOR_BLACK PICO_SCANVIDEO_PIXEL_FROM_RGB5(0, 0, 0)

uint16_t get_gradient_blue(float progress) {
    // Ensure progress is between 0.0 and 1.0
    progress = fminf(fmaxf(progress, 0.0f), 1.0f);
    uint8_t blue = (uint8_t)(255 * progress);
    return PICO_SCANVIDEO_PIXEL_FROM_RGB5(0, 0, blue);
}


#define STRIPE_ANGLE 45.0f // Angle of stripes in degrees
#define STRIPE_HEIGHT 5    // Height of each stripe
#define STRIPE_COUNT 10    // Number of stripes

// Convert degrees to radians
#define DEG_TO_RAD(deg) ((deg) * (M_PI / 180.0))

uint16_t get_stripe_blue(int stripe_index) {
    float progress = (float)(stripe_index % STRIPE_COUNT) / STRIPE_COUNT;
    uint8_t blue = (uint8_t)(255 * progress);
    return PICO_SCANVIDEO_PIXEL_FROM_RGB5(0, 0, blue);
}

void draw_xpm(const char *xpm[], int x, int y, float animation_progress) {
    int width, height, num_colors, char_size;
    sscanf(xpm[0], "%d %d %d %d", &width, &height, &num_colors, &char_size);

    // Initialize color map
    uint32_t color_map[256];
    for (int i = 0; i < 256; ++i) {
        color_map[i] = COLOR_BLACK; // Default color is black
    }
    
    // Map XPM colors to actual colors
    for (int i = 0; i < num_colors; ++i) {
        char color_char = xpm[1][i * 2];
        if (xpm[1][i * 2 + 1] == ' ') {
            color_map[(unsigned char)color_char] = COLOR_BLACK; // Black
        } else if (xpm[1][i * 2 + 1] == '.') {
            color_map[(unsigned char)color_char] = COLOR_WHITE; // White
        } else if (xpm[1][i * 2 + 1] == '+') {
            color_map[(unsigned char)color_char] = get_gradient_blue(animation_progress);  // Animated Blue
        }
    }

    float angle_rad = DEG_TO_RAD(STRIPE_ANGLE);

    // Draw the image
    for (int j = 0; j < height; ++j) {
        for (int i = 0; i < width; ++i) {
            char pixel = xpm[j + 1][i]; // Note: xpm[j + 2] because xpm[1] is the color map line
            uint16_t color = 0;

            if (pixel == ' ')
            {
                color = PICO_SCANVIDEO_PIXEL_FROM_RGB5(0, 0, 0);
            }
            else if (pixel == '.')
            {
                color = PICO_SCANVIDEO_PIXEL_FROM_RGB5(255, 255, 255);
            }
            else if (pixel == '+')
            {
                float skewed_x = i - (height - j) * tanf(angle_rad);
            int stripe_index = ((int)((skewed_x + animation_progress) / STRIPE_HEIGHT)) % STRIPE_COUNT;
                color = get_stripe_blue(stripe_index);
            }


            draw_pixel(x + i, y + j, color);
        }
    }
}

// Function to extract and draw a specific frame from the XPM data
void draw_xpm_frame(const char *xpm[], int frame_index, int x, int y) {
    int frame_width = 20;  // Width of each frame (80 / 4 = 20)
    int frame_height = 30; // Height of each frame
    int frame_offset = frame_index * frame_width; // Calculate horizontal offset for the current frame

    for (int j = 0; j < frame_height; ++j) {
        for (int i = 0; i < frame_width; ++i) {
            char pixel = xpm[j][i + frame_offset];
            uint16_t color = 0;

            // Map character to color
            switch (pixel) {
                case ' ': color = COLOR_BLACK; break;
                case '.': color = PICO_SCANVIDEO_PIXEL_FROM_RGB5(0,0,175); break;
                case '+': color = COLOR_BLUE; break;
                case '@': color = PICO_SCANVIDEO_PIXEL_FROM_RGB5(31, 49, 15); break;
                case '#': color = PICO_SCANVIDEO_PIXEL_FROM_RGB5(31, 27, 17); break;
                case '$': color = COLOR_WHITE; break;
                case '%': color = PICO_SCANVIDEO_PIXEL_FROM_RGB5(24, 24, 24); break;
                case '&': color = COLOR_BLACK; break;
            }

            draw_pixel(x + i, y + j, color);
        }
    }
}


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
    sem_init(&video_initted, 1, 1);

    // launch all the video on core 1
    multicore_launch_core1(core1_func);

    //set_clear_colour(PICO_SCANVIDEO_PIXEL_FROM_RGB5(25, 25, 25));
  
    float animation_progress = 0.0f;
    float animation_speed = 0.5f; // Adjust speed of animation

    int frame_index = 0;
    int num_frames = 4; // Total number of frames (this should be set to the number of actual frames)
    float frame_duration = 100.0f; // Duration each frame is shown, in seconds
    float time_since_last_frame = 0.0f;

    while (true) {
        absolute_time_t start_time = get_absolute_time();
        
        // Center the image
        int image_width = 96;
        int image_height = 31;

        int x = (SCREEN_WIDTH - image_width) / 2;
        int y = (SCREEN_HEIGHT - image_height) / 2;

        // Draw the XPM image at the center of the screen
        draw_xpm(sega_xpm, x, y, animation_progress);

        // Update animation progress
        animation_progress += animation_speed; // Scale with frame time
        if (animation_progress > 6.0f) {
            animation_progress -= 0.1f; // Loop the animation
        }

        image_width = 80;
        image_height = 30;

        x = ((SCREEN_WIDTH - image_width) / 2) + 90;
        y = ((SCREEN_HEIGHT - image_height) / 2);

        draw_xpm_frame(sonic_xpm, frame_index, x, y);
        
    // Update frame timing
        time_since_last_frame += frame_time;
        if (time_since_last_frame >= frame_duration) {
            frame_index = (frame_index + 1) % num_frames; // Move to the next frame
            time_since_last_frame = 0.0f; // Reset the timer for the next frame
        }

        absolute_time_t now = get_absolute_time();
        frame_time = (float)absolute_time_diff_us(start_time, now) / 1000.0f; // Convert to milliseconds
        fps = 1000.0f / frame_time;

        scanvideo_wait_for_vblank();   // Wait for vertical blanking
        swap_framebuffers(); 
    }
}
