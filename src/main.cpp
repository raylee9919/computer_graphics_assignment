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

#define RGBA_WHITE  V4(1.0f, 1.0f, 1.0f, 1.0f)
#define RGBA_BLACK  V4(0.0f, 0.0f, 0.0f, 1.0f)

#include "types.h"
#include "memory.cpp"
#include "math.cpp"
#include "gl.cpp"

b32 DEBUG_show_collision_volume;

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
    AABB collision_volume;
};
static void
init_wall(Wall *wall, v3 _position, qt _orientation, v3 collision_volume_dim)
{
    wall->position    = _position;
    wall->orientation = _orientation;
    wall->collision_volume.cen = _position;
    wall->collision_volume.dim = collision_volume_dim;
}

int main(void)
{
    Game_Memory game_memory = {};
    game_memory.permanent_memory_size = MB(64);
    game_memory.transient_memory_size = MB(512);
    size_t total_capacity = (game_memory.permanent_memory_size + 
                             game_memory.transient_memory_size);
    void *memory_base = VirtualAlloc(0, total_capacity, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    game_memory.permanent_memory = memory_base;
    game_memory.transient_memory = (u8 *)game_memory.permanent_memory + game_memory.permanent_memory_size;

    if (glfwInit())
    {
        glfwWindowHint(GLFW_SAMPLES, 1);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        GLFWwindow *window = glfwCreateWindow(1280, 720, "12191642|SungWooLee", 0, 0);
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
                init_camera(&camera, v3{0,0,0}, qt{1,0,0,0}, CameraTypePerspective, 0.5f, 0.5f, 500.0f, AABB{v3{0,0,0}, v3{2,2,2}});

                Wall walls[5];
                init_wall(&walls[0], v3{  0,-3,  0}, qt{1,0,0,0}, v3{2.2f,2.2f,2.2f});
                init_wall(&walls[1], v3{-10, 0,  0}, qt{1,0,0,0}, v3{2.2f,2.2f,2.2f});
                init_wall(&walls[2], v3{ 10, 0,  0}, qt{1,0,0,0}, v3{2.2f,2.2f,2.2f});
                init_wall(&walls[3], v3{  0, 0,-10}, qt{1,0,0,0}, v3{2.2f,2.2f,2.2f});
                init_wall(&walls[4], v3{  0, 0, 10}, qt{1,0,0,0}, v3{2.2f,2.2f,2.2f});

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

                Phong_Shader phong_shader;
                phong_shader.id = gl_create_program(shader_header, phong_vs, phong_fs);
                phong_shader.M = glGetUniformLocation(phong_shader.id, "M");
                phong_shader.VP = glGetUniformLocation(phong_shader.id, "VP");
                phong_shader.camera_pos = glGetUniformLocation(phong_shader.id, "camera_pos");

                Line_Shader line_shader;
                line_shader.id = gl_create_program(shader_header, line_vs, line_fs);
                line_shader.VP = glGetUniformLocation(line_shader.id, "VP"); 

                Memory_Arena asset_arena;
                init_arena(&asset_arena, game_memory.permanent_memory, MB(16));
                
                Mesh *cube_mesh = push_struct(&asset_arena, Mesh);
                cube_mesh->vertex_count = 8;
                cube_mesh->vertices = push_array(&asset_arena, Vertex, cube_mesh->vertex_count);
                cube_mesh->vertices[0] = Vertex{v3{-1,-1, 1}, v3{ 0, 0,-1}, v2{}, V4(1,0,0,1)};
                cube_mesh->vertices[1] = Vertex{v3{ 1,-1, 1}, v3{ 0, 0,-1}, v2{}, V4(0,1,0,1)};
                cube_mesh->vertices[2] = Vertex{v3{ 1,-1,-1}, v3{ 0, 0,-1}, v2{}, V4(0,0,1,1)};
                cube_mesh->vertices[3] = Vertex{v3{-1,-1,-1}, v3{ 0, 0,-1}, v2{}, V4(1,1,0,1)};
                cube_mesh->vertices[4] = Vertex{v3{-1, 1, 1}, v3{ 0, 0,-1}, v2{}, V4(1,0,1,1)};
                cube_mesh->vertices[5] = Vertex{v3{ 1, 1, 1}, v3{ 0, 0,-1}, v2{}, V4(0,1,1,1)};
                cube_mesh->vertices[6] = Vertex{v3{ 1, 1,-1}, v3{ 0, 0,-1}, v2{}, V4(1,1,1,1)};
                cube_mesh->vertices[7] = Vertex{v3{-1, 1,-1}, v3{ 0, 0,-1}, v2{}, V4(1,1,0,1)};
                u32 cube_indices[] = {
                    0,1,2, 0,2,3,
                    4,5,6, 4,6,7,
                    0,1,5, 0,5,4,
                    1,2,6, 1,6,5,
                    2,3,7, 2,7,6,
                    3,0,4, 3,4,7
                };
                cube_mesh->index_count = array_count(cube_indices);
                cube_mesh->indices = push_array(&asset_arena, u32, cube_mesh->index_count);
                for (u32 idx = 0; idx < array_count(cube_indices); ++idx)
                    cube_mesh->indices[idx] = cube_indices[idx];

                v2 window_dim = get_window_dimension(window);
                f64 mouse_Px, mouse_Py;
                glfwGetCursorPos(window, &mouse_Px, &mouse_Py);
                input.mouse_pos = v2{(f32)mouse_Px, window_dim.y - (f32)mouse_Py};
                f32 prev_mouse_pos_x = input.mouse_pos.x;
                f32 prev_mouse_pos_y = input.mouse_pos.y;

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
                    glfwGetCursorPos(window, &mouse_Px, &mouse_Py);
                    input.mouse_pos = v2{(f32)mouse_Px, window_dim.y - (f32)mouse_Py};

                    s32 key_state;
                    key_state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
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
                    camera.orientation = rotate(camera.orientation, v3{0,1,0}, dt * dx);
                    camera.orientation = rotate(camera.orientation, rotate(v3{1,0,0}, camera.orientation), dt * dy);
                    prev_mouse_pos_x = input.mouse_pos.x;
                    prev_mouse_pos_y = input.mouse_pos.y;

                    f32 speed = 10.0f;
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

                    //
                    // Simulate
                    //

                    // Update
                    update_camera(&camera, window_dim);
                    v3 prev_camera_pos = camera.position;

                    camera.position += dt * dir * speed;
                    for (u32 idx = 0; idx < array_count(walls); ++idx)
                    {
                        Wall wall = walls[idx];
                        if (collides(wall.collision_volume, camera.collision_volume)) {
                            camera.position = prev_camera_pos;
                        }
                    }

                    // Draw
                    for (u32 idx = 0; idx < array_count(walls); ++idx)
                    {
                        Wall *wall = walls + idx;
                        push_mesh(&render_arena, cube_mesh, to_transform(wall->position, wall->orientation));
                        if (DEBUG_show_collision_volume)
                            push_aabb(&render_arena, wall->collision_volume);
                    }

                    //
                    // Render
                    //
                    glViewport(0, 0, (u32)window_dim.x, (u32)window_dim.y);

                    glEnable(GL_BLEND);
                    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

                    glEnable(GL_CULL_FACE);
                    glCullFace(GL_BACK);
                    glFrontFace(GL_CCW);

                    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
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

                                glUseProgram(phong_shader.id);
                                glUniformMatrix4fv(phong_shader.VP, 1, GL_TRUE, &camera.VP.e[0][0]);
                                glUniform3fv(phong_shader.camera_pos, 1, (GLfloat *)&camera.position);

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
