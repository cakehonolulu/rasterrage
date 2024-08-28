#include "primitive.h"

void swap(int *a, int *b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

int drawn_triangles = 0;

void cross_product(float x1, float y1, float z1, float x2, float y2, float z2, float *nx, float *ny, float *nz) {
    *nx = y1 * z2 - z1 * y2;
    *ny = z1 * x2 - x1 * z2;
    *nz = x1 * y2 - y1 * x2;
}

float dot_product(float x1, float y1, float z1, float x2, float y2, float z2) {
    return x1 * x2 + y1 * y2 + z1 * z2;
}

void rotate_x_(float angle, float *x, float *y, float *z) {
    float cos_angle = cos(angle);
    float sin_angle = sin(angle);
    float y_new = *y * cos_angle - *z * sin_angle;
    float z_new = *y * sin_angle + *z * cos_angle;
    *y = y_new;
    *z = z_new;
}

void rotate_y_(float angle, float *x, float *y, float *z) {
    float cos_angle = cos(angle);
    float sin_angle = sin(angle);
    float x_new = *x * cos_angle + *z * sin_angle;
    float z_new = -*x * sin_angle + *z * cos_angle;
    *x = x_new;
    *z = z_new;
}

void rotate_z_(float angle, float *x, float *y, float *z) {
    float cos_angle = cos(angle);
    float sin_angle = sin(angle);
    float x_new = *x * cos_angle - *y * sin_angle;
    float y_new = *x * sin_angle + *y * cos_angle;
    *x = x_new;
    *y = y_new;
}


void rotate_x(Vec3 *p, float angle) {
    float cos_angle = cosf(angle);
    float sin_angle = sinf(angle);
    float y = p->y * cos_angle - p->z * sin_angle;
    float z = p->y * sin_angle + p->z * cos_angle;
    p->y = y;
    p->z = z;
}

void rotate_y(Vec3 *p, float angle) {
    float cos_angle = cosf(angle);
    float sin_angle = sinf(angle);
    float x = p->x * cos_angle + p->z * sin_angle;
    float z = -p->x * sin_angle + p->z * cos_angle;
    p->x = x;
    p->z = z;
}

void rotate_z(Vec3 *p, float angle) {
    float cos_angle = cosf(angle);
    float sin_angle = sinf(angle);
    float x = p->x * cos_angle - p->y * sin_angle;
    float y = p->x * sin_angle + p->y * cos_angle;
    p->x = x;
    p->y = y;
}

void translate_point(Vec3 *p, float tx, float ty) {
    p->x += tx;
    p->y += ty;
}

Vec2 project_3d_to_2d(Vec3 p) {
    Vec2 projected;
    projected.x = (p.x + SCREEN_WIDTH / 2); // Translate to screen center
    projected.y = (p.y + SCREEN_HEIGHT / 2);
    return projected;
}

void sort_faces_by_depth(Face faces[], int count) {
    int i, j;
    Face temp;
    for (i = 0; i < count - 1; i++) {
        for (j = 0; j < count - i - 1; j++) {
            if (faces[j].depth < faces[j + 1].depth) { // Descending order (furthest first)
                temp = faces[j];
                faces[j] = faces[j + 1];
                faces[j + 1] = temp;
            }
        }
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

void draw_triangle_wireframe(float x0, float y0, float x1, float y1, float x2, float y2, uint16_t color) {
    // Draw lines between each pair of vertices
    draw_line(x0, y0, x1, y1, color); // Line from vertex 0 to vertex 1
    draw_line(x1, y1, x2, y2, color); // Line from vertex 1 to vertex 2
    draw_line(x2, y2, x0, y0, color); // Line from vertex 2 to vertex 0
}

void draw_triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t color) {
    // Draw lines between each pair of vertices
    draw_line(x0, y0, x1, y1, color);
    draw_line(x1, y1, x2, y2, color);
    draw_line(x2, y2, x0, y0, color);
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

// Draw wireframe cube
void draw_wireframe_cube(Vec3 *vertices) {
    int faces[6][4] = {
        {0, 1, 2, 3}, // Back face
        {4, 5, 6, 7}, // Front face
        {0, 1, 5, 4}, // Bottom face
        {2, 3, 7, 6}, // Top face
        {0, 3, 7, 4}, // Left face
        {1, 2, 6, 5}  // Right face
    };

    for (int i = 0; i < 6; i++) {
        Vec2 p1 = project_3d_to_2d(vertices[faces[i][0]]);
        Vec2 p2 = project_3d_to_2d(vertices[faces[i][1]]);
        Vec2 p3 = project_3d_to_2d(vertices[faces[i][2]]);
        Vec2 p4 = project_3d_to_2d(vertices[faces[i][3]]);

        draw_triangle_wireframe(p1.x / 2, p1.y / 2,
                             p2.x / 2, p2.y / 2,
                             p3.x / 2, p3.y / 2, 0xFFFF);

        draw_triangle_wireframe(p3.x / 2, p3.y / 2,
                             p4.x / 2, p4.y / 2,
                             p1.x / 2, p1.y / 2, 0xFFFF);

    }
}

void draw_circle_with_lines(int centerX, int centerY, int radius) {
    int num_lines = 360; // Number of lines (one per degree)
    for (int i = 0; i < num_lines; i++) {
        float angle = (float)i * (2.0f * M_PI / (float)num_lines); // Convert degree to radians
        int x = centerX + (int)(radius * cos(angle));
        int y = centerY + (int)(radius * sin(angle));

        // Change the color with each line (cycling through a predefined color palette)
        uint16_t color = color_palette[i % 16]; // Assuming 16 colors in your palette

        draw_line(centerX, centerY, x, y, color);
    }
}


void draw_sprite(const Sprite *sprite) {
    float cos_angle = cosf(sprite->rotation);
    float sin_angle = sinf(sprite->rotation);

    // Scale and rotation adjustment
    int scaled_width = (int)(sprite->width * sprite->scale);
    int scaled_height = (int)(sprite->height * sprite->scale);

    // Adjust the sprite's center position based on the new scale
    int half_width = scaled_width / 2;
    int half_height = scaled_height / 2;

    for (int y = 0; y < scaled_height; y++) {
        for (int x = 0; x < scaled_width; x++) {
            // Map the scaled coordinates back to the original sprite coordinates
            int src_x = (int)((float)(x - half_width) / sprite->scale) + sprite->width / 2;
            int src_y = (int)((float)(y - half_height) / sprite->scale) + sprite->height / 2;

            // Apply flipping if necessary
            if (sprite->flip_x) src_x = sprite->width - 1 - src_x;
            if (sprite->flip_y) src_y = sprite->height - 1 - src_y;

            // Ensure the source coordinates are within bounds
            if (src_x >= 0 && src_x < sprite->width && src_y >= 0 && src_y < sprite->height) {
                // Apply rotation and scaling to position the sprite on the screen
                int tx = (int)(cos_angle * (x - half_width) - sin_angle * (y - half_height)) + sprite->x;
                int ty = (int)(sin_angle * (x - half_width) + cos_angle * (y - half_height)) + sprite->y;

                // Ensure the pixel is within screen bounds
                if (tx >= 0 && tx < SCREEN_WIDTH && ty >= 0 && ty < SCREEN_HEIGHT) {
                    // Ensure sprite pixel color is within valid range
                    uint8_t color_index = sprite->pixels[src_y * sprite->width + src_x];
                    if (color_index != 0xF) { // Assuming 0xF is transparent
                        draw_pixel(tx, ty, color_palette[color_index]);
                    }
                }
            }
        }
    }
}

void update_sprite_scale(Sprite *sprite) {
    static float angle = 0.0f; // Keeps track of the angle for the sine wave

    // Update the angle to animate the scaling
    angle += 0.1f; // Adjust the speed of the scaling animation
    if (angle > M_PI * 2) {
        angle -= M_PI * 2; // Keep the angle within the range [0, 2π]
    }

    // Define the minimum and maximum scale factors
    float min_scale = 0.5f;
    float max_scale = 2.5f;

    // Calculate the new scale using a sine wave
    sprite->scale = min_scale + (max_scale - min_scale) * (0.5f + 0.5f * sinf(angle));
}

void update_sprite(Sprite *sprite) {
    sprite->rotation += 0.01f; // Rotate the sprite slowly in one direction

    // Reset the rotation after a full circle
    if (sprite->rotation > M_PI * 2) {
        sprite->rotation = 0;
    }

    // Larger scaling using a sine wave, with a minimum scale of 0.5 to avoid disappearing
    float min_scale = 1.5f;
    float max_scale = 2.5f;
    sprite->scale = min_scale + (max_scale - min_scale) * (0.5f + 0.5f * sinf(sprite->rotation));  // Scales up to 2.5x and down to 0.5x
}

void draw_filled_cube(Vec3 *vertices) {
    drawn_triangles = 0;
    int faces[6][4] = {
        {0, 1, 2, 3}, // Back face
        {4, 5, 6, 7}, // Front face
        {0, 1, 5, 4}, // Bottom face
        {2, 3, 7, 6}, // Top face
        {0, 3, 7, 4}, // Left face
        {1, 2, 6, 5}  // Right face
    };

    for (int i = 0; i < 6; i++) {
        Vec2 p1 = project_3d_to_2d(vertices[faces[i][0]]);
        Vec2 p2 = project_3d_to_2d(vertices[faces[i][1]]);
        Vec2 p3 = project_3d_to_2d(vertices[faces[i][2]]);
        Vec2 p4 = project_3d_to_2d(vertices[faces[i][3]]);

        draw_filled_triangle(p1.x / 2, p1.y / 2,
                             p2.x / 2, p2.y / 2,
                             p3.x / 2, p3.y / 2, color_palette[i + 1]);

        draw_filled_triangle(p3.x / 2, p3.y / 2,
                             p4.x / 2, p4.y / 2,
                             p1.x / 2, p1.y / 2, color_palette[i + 1]);

        drawn_triangles += 2;
    }
}

// Rotate cube around X, Y, and Z axes
void rotate_cube(Vec3 *vertices, float angle_x, float angle_y, float angle_z) {
    float sin_x = sinf(angle_x), cos_x = cosf(angle_x);
    float sin_y = sinf(angle_y), cos_y = cosf(angle_y);
    float sin_z = sinf(angle_z), cos_z = cosf(angle_z);

    for (int i = 0; i < 8; i++) {
        // Translate the cube so its center is at the origin
        vertices[i].x -= CENTER_X;
        vertices[i].y -= CENTER_Y;

        // Rotate around X axis
        float y = vertices[i].y * cos_x - vertices[i].z * sin_x;
        float z = vertices[i].y * sin_x + vertices[i].z * cos_x;
        vertices[i].y = y;
        vertices[i].z = z;

        // Rotate around Y axis
        float x = vertices[i].x * cos_y + vertices[i].z * sin_y;
        z = vertices[i].z * cos_y - vertices[i].x * sin_y;
        vertices[i].x = x;
        vertices[i].z = z;

        // Rotate around Z axis
        x = vertices[i].x * cos_z - vertices[i].y * sin_z;
        y = vertices[i].x * sin_z + vertices[i].y * cos_z;
        vertices[i].x = x;
        vertices[i].y = y;

        // Translate the cube back to its original center position
        vertices[i].x += CENTER_X;
        vertices[i].y += CENTER_Y;
    }
}


// Move cube along X and Y axes
void move_cube(Vec3 *vertices, float move_x, float move_y) {
    for (int i = 0; i < 8; i++) {
        vertices[i].x += move_x;
        vertices[i].y += move_y;
    }
}

void draw_wavy_gradient() {
    // Loop through every pixel on the screen
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            // Calculate the base gradient
            float gradient = (float)(x + y) / (float)(SCREEN_WIDTH + SCREEN_HEIGHT);

            // Calculate the wave effect
            float wave = 10.0f * sin((float)x / 20.0f + (float)y / 40.0f);

            // Adjust the y-coordinate with the wave effect
            int adjusted_y = y + (int)wave;

            // Keep the y-coordinate within bounds
            if (adjusted_y >= 0 && adjusted_y < SCREEN_HEIGHT) {
                // Map the gradient to a color index
                uint16_t color = color_palette[(int)(gradient * 15.0f) % 16];

                // Set the pixel color with the wavy effect
                draw_pixel(x, adjusted_y, color);
            }
        }
    }
}


void draw_filled_circle(int cx, int cy, int radius, uint16_t color) {
    for (int y = -radius; y <= radius; y++) {
        for (int x = -radius; x <= radius; x++) {
            if (x * x + y * y <= radius * radius) {
                draw_pixel(cx + x, cy + y, color);
            }
        }
    }
}

void draw_sphere(int stacks, int sectors, int radius, int cx, int cy, float rotation, float tilt, float right_tilt) {
    drawn_triangles = 0;
    // Calculate cosine and sine of tilt angles for reuse
    float cos_tilt = cos(tilt);
    float sin_tilt = sin(tilt);
    
    float cos_right_tilt = cos(right_tilt);
    float sin_right_tilt = sin(right_tilt);

    for (int i = 0; i < stacks; i++) {
        float theta1 = PI * (-0.5 + (float)i / stacks);
        float theta2 = PI * (-0.5 + (float)(i + 1) / stacks);

        float y0 = sin(theta1);
        float y1 = sin(theta2);

        float r0 = cos(theta1);
        float r1 = cos(theta2);

        for (int j = 0; j < sectors; j++) {
            float phi1 = 2 * PI * (float)j / sectors + rotation;
            float phi2 = 2 * PI * (float)(j + 1) / sectors + rotation;

            // Original coordinates without tilt
            float x0 = cos(phi1) * r0;
            float z0 = sin(phi1) * r0;
            float x1 = cos(phi2) * r0;
            float z1 = sin(phi2) * r0;
            float x2 = cos(phi1) * r1;
            float z2 = sin(phi1) * r1;
            float x3 = cos(phi2) * r1;
            float z3 = sin(phi2) * r1;

            // Apply tilt rotation around X-axis
            float y0_tilted = y0 * cos_tilt - z0 * sin_tilt;
            float z0_tilted = y0 * sin_tilt + z0 * cos_tilt;

            float y1_tilted = y0 * cos_tilt - z1 * sin_tilt;
            float z1_tilted = y0 * sin_tilt + z1 * cos_tilt;

            float y2_tilted = y1 * cos_tilt - z2 * sin_tilt;
            float z2_tilted = y1 * sin_tilt + z2 * cos_tilt;

            float y3_tilted = y1 * cos_tilt - z3 * sin_tilt;
            float z3_tilted = y1 * sin_tilt + z3 * cos_tilt;

            // Apply rotation around Y-axis for right tilt
            float x0_final = x0 * cos_right_tilt + z0_tilted * sin_right_tilt;
            float z0_final = -x0 * sin_right_tilt + z0_tilted * cos_right_tilt;

            float x1_final = x1 * cos_right_tilt + z1_tilted * sin_right_tilt;
            float z1_final = -x1 * sin_right_tilt + z1_tilted * cos_right_tilt;

            float x2_final = x2 * cos_right_tilt + z2_tilted * sin_right_tilt;
            float z2_final = -x2 * sin_right_tilt + z2_tilted * cos_right_tilt;

            float x3_final = x3 * cos_right_tilt + z3_tilted * sin_right_tilt;
            float z3_final = -x3 * sin_right_tilt + z3_tilted * cos_right_tilt;

            // Project to screen coordinates
            int screen_x0 = (int)(cx + radius * x0_final);
            int screen_y0 = (int)(cy + radius * y0_tilted);
            int screen_x1 = (int)(cx + radius * x1_final);
            int screen_y1 = (int)(cy + radius * y1_tilted);
            int screen_x2 = (int)(cx + radius * x2_final);
            int screen_y2 = (int)(cy + radius * y2_tilted);
            int screen_x3 = (int)(cx + radius * x3_final);
            int screen_y3 = (int)(cy + radius * y3_tilted);

            // Calculate normals for the two triangles
            float nx1, ny1, nz1;
            float nx2, ny2, nz2;

            // Vector from x0 to x1
            float x01 = x1_final - x0_final;
            float y01 = y0_tilted - y0_tilted;  // y1_tilted - y0_tilted
            float z01 = z1_final - z0_final;

            // Vector from x0 to x2
            float x02 = x2_final - x0_final;
            float y02 = y2_tilted - y0_tilted;
            float z02 = z2_final - z0_final;

            // Calculate the normal for the first triangle
            cross_product(x01, y01, z01, x02, y02, z02, &nx1, &ny1, &nz1);

            // Vector from x1 to x3
            float x13 = x3_final - x1_final;
            float y13 = y3_tilted - y1_tilted;  // y3_tilted - y1_tilted
            float z13 = z3_final - z1_final;

            // Vector from x1 to x2
            float x12 = x2_final - x1_final;
            float y12 = y2_tilted - y1_tilted;
            float z12 = z2_final - z1_final;

            // Calculate the normal for the second triangle
            cross_product(x13, y13, z13, x12, y12, z12, &nx2, &ny2, &nz2);

            // View direction (towards negative Z)
            float view_dir_x = 0;
            float view_dir_y = 0;
            float view_dir_z = -1;

            // Check if the triangles face towards the viewer
            float dot = dot_product(nx1, ny1, nz1, view_dir_x, view_dir_y, view_dir_z);

            int color = (i + j) % 2 == 0 ? PICO_SCANVIDEO_PIXEL_FROM_RGB5(255, 255, 255) : PICO_SCANVIDEO_PIXEL_FROM_RGB5(255, 0, 0); // Alternate between white and red

            // Only draw triangles if they are facing towards the viewer
            if (dot > 0) {
                draw_filled_triangle(screen_x0, screen_y0, screen_x1, screen_y1, screen_x2, screen_y2, color);
                draw_filled_triangle(screen_x1, screen_y1, screen_x3, screen_y3, screen_x2, screen_y2, color);
                drawn_triangles += 2;
            }
        }
    }
}
