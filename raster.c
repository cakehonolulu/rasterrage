#include "primitive.h"
#include "framebuffer.h"

#include "rasterrage.h"

static semaphore_t video_initted;
static semaphore_t drawing_semaphore;

// Timing variables
absolute_time_t last_time;
float frame_time = 0.0f;
float fps = 0.0f;

typedef struct {
    Vec3 position;  // 3D position of the particle
    float velocity_z; // Speed towards the camera
    uint16_t color; // Color of the particle
} Particle;

#define NUM_PARTICLES 250 // Adjust this number based on performance
Particle particles[NUM_PARTICLES];

void init_particles() {
    for (int i = 0; i < NUM_PARTICLES; i++) {
        particles[i].position.x = (rand() % SCREEN_WIDTH * 2) - SCREEN_WIDTH; // Random X position within a wider range
        particles[i].position.y = (rand() % SCREEN_HEIGHT * 2) - SCREEN_HEIGHT; // Random Y position within a wider range
        particles[i].position.z = (float)(rand() % 100) + 1; // Random Z depth
        particles[i].velocity_z = 0.1f * (rand() % 10 + 1); // Random speed towards the camera
        particles[i].color = 0xFFFF; // Initial gray color (50% brightness)
    }
}

void update_particles() {
    for (int i = 0; i < NUM_PARTICLES; i++) {
        particles[i].position.z -= particles[i].velocity_z;

        // Reset the particle if it has passed the camera
        if (particles[i].position.z <= 0) {
            particles[i].position.x = (rand() % SCREEN_WIDTH * 2) - SCREEN_WIDTH;
            particles[i].position.y = (rand() % SCREEN_HEIGHT * 2) - SCREEN_HEIGHT;
            particles[i].position.z = 100.0f;
            particles[i].velocity_z = 0.1f * (rand() % 10 + 1);
        }
    }
}

Vec2 project_3d_to_2d_(Vec3 p) {
    Vec2 result;
    float scale = 100.0f / p.z; // Perspective projection scale factor
    result.x = p.x * scale + CENTER_X;
    result.y = p.y * scale + CENTER_Y;
    return result;
}

void draw_particles() {
    for (int i = 0; i < NUM_PARTICLES; i++) {
        Vec2 screen_position = project_3d_to_2d_(particles[i].position);

        int size = 1; // Size of the particle on the screen
        draw_filled_square((int)screen_position.x, (int)screen_position.y, size, particles[i].color);
    }
}

#define TORUS_MAJOR_RADIUS 30 // Major radius of the torus
#define TORUS_MINOR_RADIUS 15 // Minor radius of the torus
#define TORUS_MAJOR_SEGMENTS 12 // Number of segments along the major circle
#define TORUS_MINOR_SEGMENTS 8  // Number of segments along the minor circle

Vec3 torus_vertices[TORUS_MAJOR_SEGMENTS * TORUS_MINOR_SEGMENTS];
Face torus_faces[TORUS_MAJOR_SEGMENTS * TORUS_MINOR_SEGMENTS * 2]; // Two faces per segment

void init_torus() {
    int vertex_index = 0;
    int face_index = 0;

    for (int i = 0; i < TORUS_MAJOR_SEGMENTS; i++) {
        float theta = 2.0f * PI * i / TORUS_MAJOR_SEGMENTS;
        float cos_theta = cosf(theta);
        float sin_theta = sinf(theta);

        for (int j = 0; j < TORUS_MINOR_SEGMENTS; j++) {
            float phi = 2.0f * PI * j / TORUS_MINOR_SEGMENTS;
            float cos_phi = cosf(phi);
            float sin_phi = sinf(phi);

            torus_vertices[vertex_index].x = (TORUS_MAJOR_RADIUS + TORUS_MINOR_RADIUS * cos_phi) * cos_theta;
            torus_vertices[vertex_index].y = (TORUS_MAJOR_RADIUS + TORUS_MINOR_RADIUS * cos_phi) * sin_theta;
            torus_vertices[vertex_index].z = TORUS_MINOR_RADIUS * sin_phi;

            vertex_index++;
        }
    }

    // Define the faces
    for (int i = 0; i < TORUS_MAJOR_SEGMENTS; i++) {
        for (int j = 0; j < TORUS_MINOR_SEGMENTS; j++) {
            int next_i = (i + 1) % TORUS_MAJOR_SEGMENTS;
            int next_j = (j + 1) % TORUS_MINOR_SEGMENTS;

            int v0 = i * TORUS_MINOR_SEGMENTS + j;
            int v1 = next_i * TORUS_MINOR_SEGMENTS + j;
            int v2 = next_i * TORUS_MINOR_SEGMENTS + next_j;
            int v3 = i * TORUS_MINOR_SEGMENTS + next_j;

            torus_faces[face_index++] = (Face){{v0, v1, v2, v3}, 0.0f}; // Each face is a quad
        }
    }
}
Vec3 compute_torus_center(Vec3 *vertices) {
    Vec3 center = {0.0f, 0.0f, 0.0f};
    int num_vertices = TORUS_MAJOR_SEGMENTS * TORUS_MINOR_SEGMENTS;
    
    for (int i = 0; i < num_vertices; i++) {
        center.x += vertices[i].x;
        center.y += vertices[i].y;
        center.z += vertices[i].z;
    }

    center.x /= num_vertices;
    center.y /= num_vertices;
    center.z /= num_vertices;
    
    return center;
}

void rotate_torus(Vec3 *vertices, float angle_x, float angle_y, float angle_z) {
    for (int i = 0; i < TORUS_MAJOR_SEGMENTS * TORUS_MINOR_SEGMENTS; i++) {
        rotate_x(&vertices[i], angle_x);
        rotate_y(&vertices[i], angle_y);
        rotate_y(&vertices[i], angle_y);
    }
}

// Blends two colors based on intensity factor
uint16_t blend_color(uint16_t base_color, uint16_t light_color, float intensity) {
    // Extract color components (5 bits for red, 6 bits for green, 5 bits for blue)
    uint8_t base_r = (base_color >> 11) & 0x1F;
    uint8_t base_g = (base_color >> 5) & 0x3F;
    uint8_t base_b = base_color & 0x1F;

    uint8_t light_r = (light_color >> 11) & 0x1F;
    uint8_t light_g = (light_color >> 5) & 0x3F;
    uint8_t light_b = light_color & 0x1F;

    // Calculate blended color components
    uint8_t blended_r = (uint8_t)(base_r + (light_r - base_r) * intensity);
    uint8_t blended_g = (uint8_t)(base_g + (light_g - base_g) * intensity);
    uint8_t blended_b = (uint8_t)(base_b + (light_b - base_b) * intensity);

    // Combine components into a 16-bit color
    return (blended_r << 11) | (blended_g << 5) | blended_b;
}

void compute_normal(Vec3 *v0, Vec3 *v1, Vec3 *v2, Vec3 *normal) {
    Vec3 edge1 = {v1->x - v0->x, v1->y - v0->y, v1->z - v0->z};
    Vec3 edge2 = {v2->x - v0->x, v2->y - v0->y, v2->z - v0->z};

    // Cross product to get the normal
    normal->x = edge1.y * edge2.z - edge1.z * edge2.y;
    normal->y = edge1.z * edge2.x - edge1.x * edge2.z;
    normal->z = edge1.x * edge2.y - edge1.y * edge2.x;

    // Normalize the normal vector
    float length = sqrtf(normal->x * normal->x + normal->y * normal->y + normal->z * normal->z);
    if (length > 0) {
        normal->x /= length;
        normal->y /= length;
        normal->z /= length;
    }
}

void transform_to_camera_space(Vec3 *vertex, Vec3 *camera_vertex) {
    // Assuming a simple camera at origin looking down -Z axis
    // For more complex camera setups, apply a camera transformation matrix here
    camera_vertex->x = vertex->x;
    camera_vertex->y = vertex->y;
    camera_vertex->z = vertex->z;
}

float compute_face_depth(Face *face, Vec3 *vertices) {
    float depth = 0.0f;
    for (int i = 0; i < 4; i++) {
        depth += vertices[face->indices[i]].z;
    }
    return depth / 4.0f; // Average depth
}

int compare_faces_by_depth(const void *a, const void *b) {
    Face *face_a = (Face *)a;
    Face *face_b = (Face *)b;
    
    float depth_a = compute_face_depth(face_a, torus_vertices);
    float depth_b = compute_face_depth(face_b, torus_vertices);

    // Sort in descending order (furthest to nearest)
    return (depth_a > depth_b) ? -1 : (depth_a < depth_b) ? 1 : 0;
}

Vec3 compute_face_center(Vec3 *v0, Vec3 *v1, Vec3 *v2, Vec3 *v3) {
    Vec3 center = {
        (v0->x + v1->x + v2->x + v3->x) / 4.0f,
        (v0->y + v1->y + v2->y + v3->y) / 4.0f,
        (v0->z + v1->z + v2->z + v3->z) / 4.0f
    };
    return center;
}

void draw_torus(Vec3 *vertices, Face *faces) {
    drawn_triangles = 0;
    Vec3 view_dir = {0.0f, 0.0f, -1.0f};  // Assuming camera looks down -Z axis

    // Calculate screen center
    float screen_center_x = SCREEN_WIDTH / 2.0f;
    float screen_center_y = SCREEN_HEIGHT / 2.0f;

    // Compute the torus center
    Vec3 torus_center = compute_torus_center(vertices);

    // Translate faces based on the difference between the screen center and torus center
    float translate_x = screen_center_x - torus_center.x;
    float translate_y = screen_center_y - torus_center.y;

    // Sort faces by depth
    qsort(faces, TORUS_MAJOR_SEGMENTS * TORUS_MINOR_SEGMENTS, sizeof(Face), compare_faces_by_depth);

    for (int i = 0; i < TORUS_MAJOR_SEGMENTS * TORUS_MINOR_SEGMENTS; i++) {
        Vec3 v0_cam, v1_cam, v2_cam, v3_cam;
        transform_to_camera_space(&vertices[faces[i].indices[0]], &v0_cam);
        transform_to_camera_space(&vertices[faces[i].indices[1]], &v1_cam);
        transform_to_camera_space(&vertices[faces[i].indices[2]], &v2_cam);
        transform_to_camera_space(&vertices[faces[i].indices[3]], &v3_cam);

        // Apply translation to center the torus on the screen
        v0_cam.x += translate_x;
        v0_cam.y += translate_y;
        v1_cam.x += translate_x;
        v1_cam.y += translate_y;
        v2_cam.x += translate_x;
        v2_cam.y += translate_y;
        v3_cam.x += translate_x;
        v3_cam.y += translate_y;

        // Compute face normal in camera space
        Vec3 normal;
        compute_normal(&v0_cam, &v1_cam, &v2_cam, &normal);

        // Normalize the normal vector
        float length = sqrtf(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
        if (length > 0) {
            normal.x /= length;
            normal.y /= length;
            normal.z /= length;
        }

        // Draw filled face
        draw_filled_triangle(v0_cam.x, v0_cam.y, v1_cam.x, v1_cam.y, v2_cam.x, v2_cam.y, PICO_SCANVIDEO_PIXEL_FROM_RGB5(10, 10, 0));
        draw_filled_triangle(v2_cam.x, v2_cam.y, v3_cam.x, v3_cam.y, v0_cam.x, v0_cam.y, PICO_SCANVIDEO_PIXEL_FROM_RGB5(10, 10, 0));
        drawn_triangles += 2;

        // Draw wireframe over the filled face
        draw_line(v0_cam.x, v0_cam.y, v1_cam.x, v1_cam.y, 0xFFFF);
        draw_line(v1_cam.x, v1_cam.y, v2_cam.x, v2_cam.y, 0xFFFF);
        draw_line(v2_cam.x, v2_cam.y, v3_cam.x, v3_cam.y, 0xFFFF);
        draw_line(v3_cam.x, v3_cam.y, v0_cam.x, v0_cam.y, 0xFFFF);
    }
}


#define SCALE_FACTOR 1.5

Vec3 cube_vertices[8] = {
    {CENTER_X - 20 * SCALE_FACTOR, CENTER_Y - 20 * SCALE_FACTOR, -20 * SCALE_FACTOR},   // Vertex 0
    {CENTER_X + 20 * SCALE_FACTOR, CENTER_Y - 20 * SCALE_FACTOR, -20 * SCALE_FACTOR},   // Vertex 1
    {CENTER_X + 20 * SCALE_FACTOR, CENTER_Y + 20 * SCALE_FACTOR, -20 * SCALE_FACTOR},   // Vertex 2
    {CENTER_X - 20 * SCALE_FACTOR, CENTER_Y + 20 * SCALE_FACTOR, -20 * SCALE_FACTOR},   // Vertex 3
    {CENTER_X - 20 * SCALE_FACTOR, CENTER_Y - 20 * SCALE_FACTOR,  20 * SCALE_FACTOR},   // Vertex 4
    {CENTER_X + 20 * SCALE_FACTOR, CENTER_Y - 20 * SCALE_FACTOR,  20 * SCALE_FACTOR},   // Vertex 5
    {CENTER_X + 20 * SCALE_FACTOR, CENTER_Y + 20 * SCALE_FACTOR,  20 * SCALE_FACTOR},   // Vertex 6
    {CENTER_X - 20 * SCALE_FACTOR, CENTER_Y + 20 * SCALE_FACTOR,  20 * SCALE_FACTOR}    // Vertex 7
};


static const float SWITCH_TIME = 5.0f; // Time in seconds to switch between shapes

float elapsed_time = 0.0f;

float ball_x = SCREEN_WIDTH / 2.0f;
float ball_y = SCREEN_HEIGHT / 2.0f;
float ball_vx = 1.0f;  // Horizontal velocity
float ball_vy = 0.0f;  // Initial vertical velocity
float gravity = 0.2f;  // Constant downward force
float damping = 0.95f;  // Damping factor to simulate energy loss on bounce


typedef enum {
    POLYGON_TORUS,
    POLYGON_CUBE,
    POLYGON_SPHERE,
    POLYGON_COUNT
} PolygonType;

PolygonType current_polygon = POLYGON_TORUS;

void switch_polygon() {
    // Switch between polygons
    current_polygon = (current_polygon + 1) % POLYGON_COUNT;
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
    sem_init(&video_initted, 0, 1);

    // launch all the video on core 1
    multicore_launch_core1(core1_func);
    
    init_particles();
    init_torus();

    float angle_x = 0.02f;
    float angle_y = 0.01f;
    float angle_z = 0.01f;

    while (true) {
        absolute_time_t start_time = get_absolute_time();

        elapsed_time = (float)absolute_time_diff_us(last_time, start_time) / 1e6f; // Convert to seconds

        if (elapsed_time >= SWITCH_TIME) {
            last_time = start_time; // Reset the timer
            switch_polygon(); // Switch to the next polygon
        }

        update_particles();
        draw_particles();
        
        switch (current_polygon) {
            case POLYGON_TORUS:
                angle_y = 0.01f;
                rotate_torus(torus_vertices, angle_x, angle_y, angle_z);
                draw_torus(torus_vertices, torus_faces);
                break;
            case POLYGON_CUBE:
                angle_y = 0.01f;
                // Optionally, draw the filled cube
                rotate_cube(cube_vertices, angle_x, angle_y, angle_z);

                // Project and draw the cube
                draw_filled_cube(cube_vertices);

                // Draw the wireframe cube
                draw_wireframe_cube(cube_vertices);
                break;
            case POLYGON_SPHERE:
                float tilt_angle = 10.5f * (PI / 180.0f); 
                float right_tilt_angle = -75.5f * (PI / 180.0f);  // Tilt around Y-axis (tilt to the right)
                draw_sphere(8, 16, 25, (int)80, (int)60, angle_y, tilt_angle, right_tilt_angle);
                angle_y += 0.1f;
                break;
        }
        
        // Display frame time and FPS
        char info[40];
        snprintf(info, sizeof(info), "%d tri", drawn_triangles);
        cur_x = 0; cur_y = 0;
        write_smallstring_to_framebuffer(info, 0x0F);

        scanvideo_wait_for_vblank();
        swap_framebuffers(); 
    }
}
