R"(
layout (location = 0) in v3 vP;

uniform m4x4 M;
uniform m4x4 light_VP;

out v2 shadowmap_coord;

void main()
{
    gl_Position = light_VP * M * v4(vP, 1.0f);
    shadowmap_coord = 0.5f * ((gl_Position.xy / gl_Position.w) + v2(1.0f));
}

)";
