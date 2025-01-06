/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Sung Woo Lee $
   $Notice: (C) Copyright 2024 by Sung Woo Lee. All Rights Reserved. $
   ======================================================================== */
#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <intrin.h>

#include "include/GL/glew.h"
#include "include/GLFW/glfw3.h"

#define ASSERT(EXP) if (!(EXP)) *(volatile int *)0 = 0
#define INVALID_CODE_PATH ASSERT(!"InvalidCodePath")
#define INVALID_DEFAULT_CASE default: { ASSERT(!"InvalidDefaultCase"); } break;
#define array_count(ARR) (sizeof(ARR) / sizeof(ARR[0]))
#define offset_of(STRUCT, MEMBER) (size_t)&(((STRUCT *)0)->MEMBER)
#define MAX(a, b) (a > b ? a : b)
#define MIN(a, b) (a > b ? b : a)

#define RGBA_WHITE  V4(1.0f, 1.0f, 1.0f, 1.0f)
#define RGBA_BLACK  V4(0.0f, 0.0f, 0.0f, 1.0f)

#include "types.h"
#include "memory.cpp"
#include "math.cpp"
#include "asset.cpp"
#include "gl.cpp"

b32 DEBUG_show_collision_volume;
u32 global_next_bullet_idx = 0;

struct Game_Memory 
{
    void *permanent_memory;
    size_t permanent_memory_size;

    void *transient_memory;
    size_t transient_memory_size;
};

static v2
get_window_dimension(GLFWwindow *window)
{
    s32 iW, iH;
    f32 W, H;
    glfwGetWindowSize(window, &iW, &iH);
    W = (f32)iW;
    H = (f32)iH;

    v2 result = v2{W, H};
    return result;
}

enum Key
{
    KeyTilde,
    KeyMouseLeft,
    KeyCount,
};
struct Input
{
    v2 mouse_pos;
    b32 toggled[KeyCount];
    b32 is_down[KeyCount];
};
inline b32
toggled_down(Input *input, Key button)
{
    b32 result = (input->is_down[button] && input->toggled[button]);
    return result;
}
inline b32
toggled_up(Input *input, Key button)
{
    b32 result = (!input->is_down[button] && input->toggled[button]);
    return result;
}

struct Vertex
{
    v3 pos;
    v3 normal;
    v2 uv;
    v4 color;
};

struct Mesh
{
    u32     vertex_count;
    Vertex  *vertices;

    u32     index_count;
    u32     *indices;
};

enum Render_Entity_Type
{
    RenderEntityTypeMesh,
    RenderEntityTypeAABB,
};

struct Render_Entity_Header
{
    Render_Entity_Type type;
    size_t size;
};

struct Render_Entity_Mesh
{
    Render_Entity_Header header;
    Mesh *mesh;
    m4x4 M;
};

struct Render_Entity_AABB
{
    Render_Entity_Header header;
    AABB aabb;
};

#define push_render_entity(ARENA, TYPE) ((Render_Entity_##TYPE *)_push_render_entity(ARENA, RenderEntityType##TYPE, sizeof(Render_Entity_##TYPE)))
static void *
_push_render_entity(Memory_Arena *arena, Render_Entity_Type type, size_t size)
{
    Render_Entity_Header *header = (Render_Entity_Header *)push_size(arena, size);
    header->type = type;
    header->size = size;
    return header;
}

static void
push_mesh(Memory_Arena *arena, Mesh *mesh, m4x4 M)
{
   Render_Entity_Mesh *data = push_render_entity(arena, Mesh);
   data->mesh = mesh;
   data->M = M;
}

static void
push_aabb(Memory_Arena *arena, AABB aabb)
{
   Render_Entity_AABB *data = push_render_entity(arena, AABB);
   data->aabb = aabb;
}

enum Camera_Type 
{
    CameraTypePerspective,
    CameraTypeOrthographic,
};

struct Camera
{
    v3 position;
    qt orientation;

    Camera_Type type;
    f32 focal_length;
    f32 width;
    f32 height;
    f32 N;
    f32 F;

    m4x4 VP;

    AABB collision_volume;
};

static void
init_camera(Camera *camera, v3 _position, qt _orientation, Camera_Type _type, f32 _focal_length, f32 _N, f32 _F, AABB collision_volume)
{
    camera->position     = _position;
    camera->orientation  = _orientation;
    camera->type         = _type;
    camera->focal_length = _focal_length;
    camera->N            = _N;
    camera->F            = _F;

    camera->collision_volume = collision_volume;
}

static void
update_camera(Camera *camera, v2 dim)
{
    if (camera->type == CameraTypePerspective) {
        camera->width  = 2.0f; // @Temporary
        camera->height = camera->width * (dim.y / dim.x);

        m4x4 camera_rotation = to_m4x4(camera->orientation);
        m4x4 V = camera_transform(get_column(camera_rotation, 0),
                                  get_column(camera_rotation, 1),
                                  get_column(camera_rotation, 2),
                                  camera->position);
        f32 f = camera->focal_length;
        f32 N = camera->N;
        f32 F = camera->F;
        f32 a = safe_ratio(2.0f * f, camera->width);
        f32 b = safe_ratio(2.0f * f, camera->height);
        f32 c = (N + F) / (N - F);
        f32 d = (2 * N * F) / (N - F);
        m4x4 P = {{
            { a,  0,  0,  0},
            { 0,  b,  0,  0},
            { 0,  0,  c,  d},
            { 0,  0, -1,  0}
        }};

        camera->VP = P * V;
    }
    else {
        INVALID_CODE_PATH;
    }

    camera->collision_volume.cen = camera->position;
}

struct Wall
{
    v3 position;
    qt orientation;
    v3 scale;
    AABB collision_volume;
};

static void
init_wall(Wall *wall, v3 position, qt orientation, v3 scale, v3 collision_volume_dim)
{
    wall->position    = position;
    wall->orientation = orientation;
    wall->scale       = scale;
    wall->collision_volume.cen = position;
    wall->collision_volume.dim = collision_volume_dim;
}

struct Bullet
{
    v3 position;
    qt orientation;

    AABB collision_volume;

    v3 accel;
    v3 velocity;

    f32 remain_lifetime;
};

static void
init_bullet(Bullet *bullet, v3 position, qt orientation, v3 collision_volume_dim)
{
    bullet->position = position;
    bullet->orientation = orientation;
    bullet->collision_volume.cen = position;
    bullet->collision_volume.dim = collision_volume_dim;

    bullet->velocity = noz((to_m4x4(bullet->orientation) * V4(0,0,-1,1)).xyz) * 50.0f;
    bullet->remain_lifetime = 10.0f;
}

static void
update_bullet(Bullet *bullet, f32 dt)
{
    if (bullet->remain_lifetime > 0.0f)
    {
        bullet->accel = V3(0, -9.8f, 0);
        bullet->velocity += dt * bullet->accel;
        bullet->position += dt * bullet->velocity;
        bullet->remain_lifetime -= dt;
    }
}

static void
gl_alloc_texture(Bitmap *bitmap, GLenum pname)
{
    glGenTextures(1, &bitmap->handle);
    glBindTexture(GL_TEXTURE_2D, bitmap->handle);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, bitmap->width, bitmap->height,
                 0, GL_RGBA, GL_UNSIGNED_BYTE, bitmap->memory);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, pname);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, pname);
}

static void
gl_bind_texture(Bitmap *bitmap)
{
    if (bitmap->handle) {
        glBindTexture(GL_TEXTURE_2D, bitmap->handle);
    } else {
        gl_alloc_texture(bitmap, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, bitmap->handle);
    }
}

int main(void)
{
    Game_Memory game_memory = {};
    game_memory.permanent_memory_size = MB(64);
    game_memory.transient_memory_size = MB(512);
    size_t total_capacity = (game_memory.permanent_memory_size + 
                             game_memory.transient_memory_size);
    void *memory_base = malloc(total_capacity);
    game_memory.permanent_memory = memory_base;
    game_memory.transient_memory = (u8 *)game_memory.permanent_memory + game_memory.permanent_memory_size;

    if (glfwInit())
    {
        glfwWindowHint(GLFW_SAMPLES, 1);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        GLFWwindow *window = glfwCreateWindow(1600, 900, "12191642|SungWooLee", 0, 0);
        if (window)
        {
            glfwMakeContextCurrent(window);
            glewExperimental = true; // Needed for core profile

            if (glewInit() == GLEW_OK) 
            { 
                // Ensure we can capture the escape key being pressed below
                glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

                u32 vao; // dummy
                glGenVertexArrays(1, &vao);
                glBindVertexArray(vao);

                u32 vbo;
                glGenBuffers(1, &vbo);
                glBindBuffer(GL_ARRAY_BUFFER, vbo);

                u32 vio;
                glGenBuffers(1, &vio);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vio);

                Input input = {};
                f32 prev_time = 0;
                b32 should_close = false;

                Camera camera;
                init_camera(&camera, v3{0,0,0}, qt{1,0,0,0}, CameraTypePerspective, 0.5f, 0.5f, 1000.0f, AABB{v3{0,0,0}, V3(1.5f)});
                const f32 speed = 2.0f;

                Wall walls[8];
                init_wall(&walls[0], V3(  0,-2,  0), qt{1,0,0,0}, V3(10.0f, 1.0f, 10.0f), V3(18.0f, 2.0f, 18.0f));
                init_wall(&walls[1], V3(-10, 0,  0), qt{1,0,0,0}, V3(1.0f,  1.0f, 10.0f), V3(2.0f,  2.0f, 18.0f));
                init_wall(&walls[2], V3( 10, 0,  0), qt{1,0,0,0}, V3(1.0f,  1.0f, 10.0f), V3(2.0f,  2.0f, 18.0f));
                init_wall(&walls[3], V3(  0, 0,-10), qt{1,0,0,0}, V3(10.0f, 1.0f, 1.0f),  V3(18.0f, 2.0f, 2.0f));
                init_wall(&walls[4], V3(  0, 0, 10), qt{1,0,0,0}, V3(10.0f, 1.0f, 1.0f),  V3(18.0f, 2.0f, 2.0f));

                init_wall(&walls[5], V3( 1, 0, 2), qt{1,0,0,0}, V3(0.5f), V3(1.0f));
                init_wall(&walls[6], V3( 3, 0, 0), qt{1,0,0,0}, V3(0.5f), V3(1.0f));
                init_wall(&walls[7], V3( 0, 0, 3), qt{1,0,0,0}, V3(0.5f), V3(1.0f));

                Bullet bullets[256];
                for (u32 idx = 0; idx < array_count(bullets); ++idx)
                    bullets[idx] = Bullet{};

                v3 directional_light_dir   = noz(V3(1, 1, 0));
                v3 directional_light_color = V3(1, 1, 1);
                u32 shadowmap_fbo;
                glGenFramebuffers(1, &shadowmap_fbo);
                u32 shadowmap;
                u32 shadowmap_width = 1024, shadowmap_height = 1024;
                glGenTextures(1, &shadowmap);
                glBindTexture(GL_TEXTURE_2D, shadowmap);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, shadowmap_width, shadowmap_height,
                             0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

                const char *shader_header = 
                    #include "shader/shader.header"
                const char *phong_vs = 
                    #include "shader/phong.vs"
                const char *phong_fs = 
                    #include "shader/phong.fs"
                const char *line_vs = 
                    #include "shader/line.vs"
                const char *line_fs = 
                    #include "shader/line.fs"
                const char *bitmap_vs = 
                    #include "shader/bitmap.vs"
                const char *bitmap_fs = 
                    #include "shader/bitmap.fs"
                const char *shadowmap_vs = 
                    #include "shader/shadowmap.vs"
                const char *shadowmap_fs = 
                    #include "shader/shadowmap.fs"

                Phong_Shader phong_shader;
                phong_shader.id = gl_create_program(shader_header, phong_vs, phong_fs);
                phong_shader.M = glGetUniformLocation(phong_shader.id, "M");
                phong_shader.VP = glGetUniformLocation(phong_shader.id, "VP");
                phong_shader.camera_pos = glGetUniformLocation(phong_shader.id, "camera_pos");
                phong_shader.directional_light_dir = glGetUniformLocation(phong_shader.id, "directional_light_dir");
                phong_shader.directional_light_color = glGetUniformLocation(phong_shader.id, "directional_light_color");

                Line_Shader line_shader;
                line_shader.id = gl_create_program(shader_header, line_vs, line_fs);
                line_shader.VP = glGetUniformLocation(line_shader.id, "VP"); 

                Bitmap_Shader bitmap_shader;
                bitmap_shader.id = gl_create_program(shader_header, bitmap_vs, bitmap_fs);

                Shadowmap_Shader shadowmap_shader;
                shadowmap_shader.id = gl_create_program(shader_header, shadowmap_vs, shadowmap_fs);
                shadowmap_shader.M = glGetUniformLocation(shadowmap_shader.id, "M"); 
                shadowmap_shader.light_VP = glGetUniformLocation(shadowmap_shader.id, "light_VP"); 

                Memory_Arena asset_arena;
                init_arena(&asset_arena, game_memory.permanent_memory, MB(16));
                
                Mesh *cube_mesh = push_struct(&asset_arena, Mesh);
                cube_mesh->vertex_count = 24;
                cube_mesh->vertices = push_array(&asset_arena, Vertex, cube_mesh->vertex_count);
                cube_mesh->vertices[0]  = Vertex{v3{-1, -1,  1}, v3{ 0,  0,  1}, v2{}, RGBA_WHITE};
                cube_mesh->vertices[1]  = Vertex{v3{ 1, -1,  1}, v3{ 0,  0,  1}, v2{}, RGBA_WHITE};
                cube_mesh->vertices[2]  = Vertex{v3{ 1,  1,  1}, v3{ 0,  0,  1}, v2{}, RGBA_WHITE};
                cube_mesh->vertices[3]  = Vertex{v3{-1,  1,  1}, v3{ 0,  0,  1}, v2{}, RGBA_WHITE};

                cube_mesh->vertices[4]  = Vertex{v3{ 1, -1, -1}, v3{ 0,  0, -1}, v2{}, RGBA_WHITE};
                cube_mesh->vertices[5]  = Vertex{v3{-1, -1, -1}, v3{ 0,  0, -1}, v2{}, RGBA_WHITE};
                cube_mesh->vertices[6]  = Vertex{v3{-1,  1, -1}, v3{ 0,  0, -1}, v2{}, RGBA_WHITE};
                cube_mesh->vertices[7]  = Vertex{v3{ 1,  1, -1}, v3{ 0,  0, -1}, v2{}, RGBA_WHITE};

                cube_mesh->vertices[8]  = Vertex{v3{-1, -1, -1}, v3{-1,  0,  0}, v2{}, RGBA_WHITE};
                cube_mesh->vertices[9]  = Vertex{v3{-1, -1,  1}, v3{-1,  0,  0}, v2{}, RGBA_WHITE};
                cube_mesh->vertices[10] = Vertex{v3{-1,  1,  1}, v3{-1,  0,  0}, v2{}, RGBA_WHITE};
                cube_mesh->vertices[11] = Vertex{v3{-1,  1, -1}, v3{-1,  0,  0}, v2{}, RGBA_WHITE};

                cube_mesh->vertices[12] = Vertex{v3{ 1, -1,  1}, v3{ 1,  0,  0}, v2{}, RGBA_WHITE};
                cube_mesh->vertices[13] = Vertex{v3{ 1, -1, -1}, v3{ 1,  0,  0}, v2{}, RGBA_WHITE};
                cube_mesh->vertices[14] = Vertex{v3{ 1,  1, -1}, v3{ 1,  0,  0}, v2{}, RGBA_WHITE};
                cube_mesh->vertices[15] = Vertex{v3{ 1,  1,  1}, v3{ 1,  0,  0}, v2{}, RGBA_WHITE};

                cube_mesh->vertices[16] = Vertex{v3{-1,  1,  1}, v3{ 0,  1,  0}, v2{}, RGBA_WHITE};
                cube_mesh->vertices[17] = Vertex{v3{ 1,  1,  1}, v3{ 0,  1,  0}, v2{}, RGBA_WHITE};
                cube_mesh->vertices[18] = Vertex{v3{ 1,  1, -1}, v3{ 0,  1,  0}, v2{}, RGBA_WHITE};
                cube_mesh->vertices[19] = Vertex{v3{-1,  1, -1}, v3{ 0,  1,  0}, v2{}, RGBA_WHITE};

                cube_mesh->vertices[20] = Vertex{v3{-1, -1, -1}, v3{ 0, -1,  0}, v2{}, RGBA_WHITE};
                cube_mesh->vertices[21] = Vertex{v3{ 1, -1, -1}, v3{ 0, -1,  0}, v2{}, RGBA_WHITE};
                cube_mesh->vertices[22] = Vertex{v3{ 1, -1,  1}, v3{ 0, -1,  0}, v2{}, RGBA_WHITE};
                cube_mesh->vertices[23] = Vertex{v3{-1, -1,  1}, v3{ 0, -1,  0}, v2{}, RGBA_WHITE};
                u32 cube_indices[] = {
                    0, 1, 2,  0, 2, 3,
                    4, 5, 6,  4, 6, 7,
                    8, 9, 10, 8, 10,11,
                    12,13,14, 12,14,15,
                    16,17,18, 16,18,19,
                    20,21,22, 20,22,23
                };
                cube_mesh->index_count = array_count(cube_indices);
                cube_mesh->indices = push_array(&asset_arena, u32, cube_mesh->index_count);
                for (u32 idx = 0; idx < array_count(cube_indices); ++idx)
                    cube_mesh->indices[idx] = cube_indices[idx];

                v2 window_dim = get_window_dimension(window);
                f64 mouse_pos_x, mouse_pos_y;
                glfwGetCursorPos(window, &mouse_pos_x, &mouse_pos_y);
                input.mouse_pos = v2{(f32)mouse_pos_x, window_dim.y - (f32)mouse_pos_y};
                f32 prev_mouse_pos_x = input.mouse_pos.x;
                f32 prev_mouse_pos_y = input.mouse_pos.y;

                Bitmap *rifle_bitmap = load_bmp(&asset_arena, "rifle.bmp");

                while (!should_close)
                {
                    Memory_Arena render_arena;
                    init_arena(&render_arena, game_memory.transient_memory, game_memory.transient_memory_size);

                    //
                    // Process Inputs.
                    //
                    f32 cur_time = (f32)glfwGetTime();
                    f32 dt = cur_time - prev_time;
                    prev_time = cur_time;

                    if (glfwWindowShouldClose(window))
                        should_close = true;

                    // Get window dimension.
                    window_dim = get_window_dimension(window);

                    // Get mouse info.
                    glfwGetCursorPos(window, &mouse_pos_x, &mouse_pos_y);
                    input.mouse_pos = v2{(f32)mouse_pos_x, window_dim.y - (f32)mouse_pos_y};

                    s32 key_state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
                    if (key_state == GLFW_RELEASE) {
                        input.toggled[KeyMouseLeft] = (input.is_down[KeyMouseLeft]) ? 1 : 0;
                        input.is_down[KeyMouseLeft] = 0;
                    }
                    else if (key_state == GLFW_PRESS) {
                        input.toggled[KeyMouseLeft] = (!input.is_down[KeyMouseLeft]) ? 1 : 0;
                        input.is_down[KeyMouseLeft] = 1;
                    }
                    key_state = glfwGetKey(window, GLFW_KEY_GRAVE_ACCENT);
                    if (key_state == GLFW_RELEASE) {
                        input.toggled[KeyTilde] = (input.is_down[KeyTilde]) ? 1 : 0;
                        input.is_down[KeyTilde] = 0;
                    }
                    else if (key_state == GLFW_PRESS) {
                        input.toggled[KeyTilde] = (!input.is_down[KeyTilde]) ? 1 : 0;
                        input.is_down[KeyTilde] = 1;
                    }

                    f32 dx = prev_mouse_pos_x - input.mouse_pos.x;
                    f32 dy = input.mouse_pos.y - prev_mouse_pos_y;
                    camera.orientation = rotate(camera.orientation, v3{0,1,0}, dt*dx);
                    camera.orientation = rotate(camera.orientation, rotate(v3{1,0,0}, camera.orientation), dt*dy);
                    prev_mouse_pos_x = input.mouse_pos.x;
                    prev_mouse_pos_y = input.mouse_pos.y;

                    v3 dir = v3{};
                    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
                        dir += (to_m4x4(camera.orientation) * V4(0,0,-1,1)).xyz;
                    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
                        dir += (to_m4x4(camera.orientation) * V4(0,0,1,1)).xyz;
                    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
                        dir += (to_m4x4(camera.orientation) * V4(-1,0,0,1)).xyz;
                    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
                        dir += (to_m4x4(camera.orientation) * V4(1,0,0,1)).xyz;
                    dir = noz(v3{dir.x, 0, dir.z});

                    if (toggled_down(&input, KeyTilde))
                    {
                        DEBUG_show_collision_volume = !DEBUG_show_collision_volume;
                        printf("[DEBUG] show_collision_volume: %d\n", DEBUG_show_collision_volume);
                    }

                    if (toggled_down(&input, KeyMouseLeft))
                    {
                        Bullet *bullet = bullets + global_next_bullet_idx;
                        global_next_bullet_idx = (global_next_bullet_idx + 1) % array_count(bullets);
                        v3 offset = noz((to_m4x4(camera.orientation) * V4(1,-0.5f,0,1)).xyz) * 0.5f;
                        init_bullet(bullet, camera.position + offset, camera.orientation, V3(1));
                    }

                    //
                    // Simulate
                    //

                    // Update
                    update_camera(&camera, window_dim);
                    for (u32 idx = 0; idx < array_count(bullets); ++idx)
                    {
                        Bullet *bullet = bullets + idx;
                        update_bullet(bullet, dt);
                    }

                    v3 prev_pos = camera.position;
                    camera.position += (dt * speed * dir);
                    for (u32 idx = 0; idx < array_count(walls); ++idx)
                    {
                        Wall wall = walls[idx];
                        AABB aabb = wall.collision_volume;
                        aabb.dim += camera.collision_volume.dim;
                        if (is_in(camera.position, aabb)) 
                        {
                            camera.position = prev_pos;
                            break;
                        }
                    }

                    // Draw
                    for (u32 idx = 0; idx < array_count(walls); ++idx)
                    {
                        Wall *wall = walls + idx;
                        push_mesh(&render_arena, cube_mesh, to_transform(wall->position, wall->orientation, wall->scale));
                        if (DEBUG_show_collision_volume)
                            push_aabb(&render_arena, wall->collision_volume);
                    }

                    for (u32 idx = 0; idx < array_count(bullets); ++idx)
                    {
                        Bullet *bullet = bullets + idx;
                        if (bullet->remain_lifetime > 0.0f)
                        {
                            push_mesh(&render_arena, cube_mesh, to_transform(bullet->position, bullet->orientation, V3(1.0f)));
                        }
                    }

                    //
                    // Render
                    //

                    // Shadow Map Pass
                    glViewport(0, 0, shadowmap_width, shadowmap_height);
                    glBindFramebuffer(GL_FRAMEBUFFER, shadowmap_fbo);
                    glClearDepth(1.0f);
                    glClear(GL_DEPTH_BUFFER_BIT);
                    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowmap, 0);
                    glDrawBuffer(GL_NONE);
                    glReadBuffer(GL_NONE);

                    f32 a = 1.0f / 10.0f;
                    f32 b = 1.0f / 10.0f;
                    m4x4 light_P = {{
                        { a,  0,  0,  0},
                        { 0,  a,  0,  0},
                        { 0,  0,  b,  0},
                        { 0,  0,  0,  1}
                    }};
                    m4x4 light_V = identity();
                    m4x4 light_VP = light_P * light_V;

                    glUseProgram(shadowmap_shader.id);

                    for (u8 *at = render_arena.base;
                         at < render_arena.base + render_arena.used;)
                    {
                        Render_Entity_Header *header = (Render_Entity_Header *)at;
                        switch (header->type) 
                        {
                            case RenderEntityTypeMesh:
                            {
                                Render_Entity_Mesh *data = (Render_Entity_Mesh *)at;

                                glUniformMatrix4fv(shadowmap_shader.M, 1, GL_TRUE, &data->M.e[0][0]);
                                glUniformMatrix4fv(shadowmap_shader.light_VP, 1, GL_TRUE, &light_VP.e[0][0]);

                                glEnableVertexAttribArray(0);

                                glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(Vertex), (GLvoid *)(offset_of(Vertex, pos)));

                                glBufferData(GL_ARRAY_BUFFER,
                                             data->mesh->vertex_count * sizeof(Vertex),
                                             data->mesh->vertices,
                                             GL_DYNAMIC_DRAW);

                                glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                                             data->mesh->index_count * sizeof(u32),
                                             data->mesh->indices,
                                             GL_DYNAMIC_DRAW);

                                glDrawElements(GL_TRIANGLES, data->mesh->index_count, GL_UNSIGNED_INT, (void *)0);

                                glDisableVertexAttribArray(0);
                            } break;

                            default:
                            {
                            } break;
                        }

                        at += header->size;
                    }
            
                    // Phong Pass
                    glViewport(0, 0, (u32)window_dim.x, (u32)window_dim.y);
                    glBindFramebuffer(GL_FRAMEBUFFER, 0);

                    glEnable(GL_BLEND);
                    glBlendFunc(GL_ONE, GL_ONE_MINUS_DST_ALPHA);

                    glEnable(GL_CULL_FACE);
                    glCullFace(GL_BACK);
                    glFrontFace(GL_CCW);

                    glClearColor(0.73f, 0.84f, 0.96f, 1.0f);
                    glClear(GL_COLOR_BUFFER_BIT);

                    glEnable(GL_DEPTH_TEST);
                    glDepthFunc(GL_LEQUAL);
                    glClearDepth(1.0f);
                    glClear(GL_DEPTH_BUFFER_BIT);

                    u8 *at = render_arena.base;
                    u8 *end = render_arena.base + render_arena.used;
                    while (at != end) 
                    {
                        Render_Entity_Header *header = (Render_Entity_Header *)at;
                        switch (header->type) 
                        {
                            case RenderEntityTypeMesh:
                            {
                                Render_Entity_Mesh *data = (Render_Entity_Mesh *)at;

                                glEnable(GL_DEPTH_TEST);

                                glUseProgram(phong_shader.id);

                                glBindTexture(GL_TEXTURE_2D, shadowmap);

                                glUniformMatrix4fv(phong_shader.VP, 1, GL_TRUE, &camera.VP.e[0][0]);
                                glUniform3fv(phong_shader.camera_pos, 1, (GLfloat *)&camera.position);
                                glUniform3fv(phong_shader.directional_light_dir, 1, (GLfloat *)&directional_light_dir);
                                glUniform3fv(phong_shader.directional_light_color, 1, (GLfloat *)&directional_light_color);

                                glUniformMatrix4fv(phong_shader.M, 1, GL_TRUE, &data->M.e[0][0]);

                                glEnableVertexAttribArray(0);
                                glEnableVertexAttribArray(1);
                                glEnableVertexAttribArray(2);
                                glEnableVertexAttribArray(3);

                                glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(Vertex), (GLvoid *)(offset_of(Vertex, pos)));
                                glVertexAttribPointer(1, 3, GL_FLOAT, false, sizeof(Vertex), (GLvoid *)(offset_of(Vertex, normal)));
                                glVertexAttribPointer(2, 2, GL_FLOAT, false, sizeof(Vertex), (GLvoid *)(offset_of(Vertex, uv)));
                                glVertexAttribPointer(3, 4, GL_FLOAT, true,  sizeof(Vertex), (GLvoid *)(offset_of(Vertex, color)));

                                glBufferData(GL_ARRAY_BUFFER,
                                             data->mesh->vertex_count * sizeof(Vertex),
                                             data->mesh->vertices,
                                             GL_DYNAMIC_DRAW);

                                glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                                             data->mesh->index_count * sizeof(u32),
                                             data->mesh->indices,
                                             GL_DYNAMIC_DRAW);

                                glDrawElements(GL_TRIANGLES, data->mesh->index_count, GL_UNSIGNED_INT, (void *)0);

                                glDisableVertexAttribArray(0);
                                glDisableVertexAttribArray(1);
                                glDisableVertexAttribArray(2);
                                glDisableVertexAttribArray(3);
                            } break;

                            case RenderEntityTypeAABB:
                            {
                                Render_Entity_AABB *data = (Render_Entity_AABB *)at;
                                AABB aabb = data->aabb;

                                glDisable(GL_DEPTH_TEST);

                                glUseProgram(line_shader.id);
                                glUniformMatrix4fv(line_shader.VP, 1, GL_TRUE, &camera.VP.e[0][0]);

                                glEnableVertexAttribArray(0);
                                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(v3), (GLvoid *)0);

                                v3 hd = aabb.dim * 0.5f;
                                v3 pos[] = {
                                    v3{aabb.cen.x - hd.x, aabb.cen.y - hd.y, aabb.cen.z + hd.z},
                                    v3{aabb.cen.x + hd.x, aabb.cen.y - hd.y, aabb.cen.z + hd.z},
                                    v3{aabb.cen.x + hd.x, aabb.cen.y - hd.y, aabb.cen.z - hd.z},
                                    v3{aabb.cen.x - hd.x, aabb.cen.y - hd.y, aabb.cen.z - hd.z},
                                    v3{aabb.cen.x - hd.x, aabb.cen.y + hd.y, aabb.cen.z + hd.z},
                                    v3{aabb.cen.x + hd.x, aabb.cen.y + hd.y, aabb.cen.z + hd.z},
                                    v3{aabb.cen.x + hd.x, aabb.cen.y + hd.y, aabb.cen.z - hd.z},
                                    v3{aabb.cen.x - hd.x, aabb.cen.y + hd.y, aabb.cen.z - hd.z},
                                };
                                u32 indices[] = {
                                    0, 1, 1, 2, 2, 3, 3, 0,
                                    4, 5, 5, 6, 6, 7, 7, 4,
                                    0, 4, 1, 5, 2, 6, 3, 7
                                };

                                glBufferData(GL_ARRAY_BUFFER, array_count(pos) * sizeof(v3), pos, GL_DYNAMIC_DRAW);
                                glBufferData(GL_ELEMENT_ARRAY_BUFFER, array_count(indices) * sizeof(u32), indices, GL_DYNAMIC_DRAW);

                                glDrawElements(GL_LINES, array_count(indices), GL_UNSIGNED_INT, 0);

                                glDisableVertexAttribArray(0);
                            } break;

                            INVALID_DEFAULT_CASE;
                        }

                        at += header->size;
                    }

                    // Gun Bitmap
                    gl_bind_texture(rifle_bitmap);
                    glUseProgram(bitmap_shader.id);
                    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

                    glfwSwapBuffers(window);
                    glfwPollEvents();
                }
            }
            else
            {
                fprintf(stderr, "[ERROR]: Couldn't init glew.\n");
                return 1;
            }
        }
        else
        {
            fprintf(stderr, "[ERROR]: Couldn't create window.\n");
            return 1;
        }
    }
    else
    {
        fprintf(stderr, "[ERROR]: Couldn't init glfw.\n");
        return 1;
    }

    fprintf(stderr, "[OK]: Program ended successfully.\n");
    return 0;
}
