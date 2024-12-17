R"(
uniform m4x4  VP;

layout (location = 0) in v3 pos;

void main()
{
    gl_Position = VP * v4(pos, 1.0);
}

)";
