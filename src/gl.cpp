/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Sung Woo Lee $
   $Notice: (C) Copyright 2024 by Sung Woo Lee. All Rights Reserved. $
   ======================================================================== */



struct Phong_Shader
{
    u32 id;
    u32 M;
    u32 VP;
    u32 camera_pos;
    u32 directional_light_dir;
    u32 directional_light_color;
};

struct Bitmap_Shader
{
    u32 id;
};

struct Line_Shader
{
    u32 id;
    u32 VP;
};

struct Shadowmap_Shader
{
    u32 id;
    u32 M;
    u32 light_VP;
};

static GLuint
gl_create_program(const char *header,
                  const char *vsrc,
                  const char *fsrc)
{
    GLuint program = 0;

    if (glCreateShader) 
    {
        GLuint vshader = glCreateShader(GL_VERTEX_SHADER);
        const GLchar *vunit[] = {
            header,
            vsrc
        };
        glShaderSource(vshader, array_count(vunit), (const GLchar **)vunit, 0);
        glCompileShader(vshader);

        GLuint fshader = glCreateShader(GL_FRAGMENT_SHADER);
        const GLchar *funit[] = {
            header,
            fsrc
        };
        glShaderSource(fshader, array_count(funit), (const GLchar **)funit, 0);
        glCompileShader(fshader);

        program = glCreateProgram();
        glAttachShader(program, vshader);
        glAttachShader(program, fshader);
        glLinkProgram(program);

        glValidateProgram(program);
        GLint linked = false;
        glGetProgramiv(program, GL_LINK_STATUS, &linked);
        if (!linked) 
        {
            GLsizei stub;

            GLchar vlog[1024];
            glGetShaderInfoLog(vshader, sizeof(vlog), &stub, vlog);

            GLchar flog[1024];
            glGetShaderInfoLog(fshader, sizeof(flog), &stub, flog);

            GLchar plog[1024];
            glGetProgramInfoLog(program, sizeof(plog), &stub, plog);

            ASSERT(!"compile/link error.");
        }
    } 
    else 
    {
        // TODO: handling.
    }
    
    return program;
}

