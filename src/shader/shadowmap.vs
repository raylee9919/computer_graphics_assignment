R"(
layout (location = 0) in v3 vP;

uniform m4x4 M;
uniform m4x4 light_VP;

void main()
{
    gl_Position = light_VP * M * v4(vP, 1.0f);
}

)";
