R"(

#define LIGHT_ATTENUATION_CONSTANT  0.6f
#define LIGHT_ATTENUATION_LINEAR    0.3f
#define LIGHT_ATTENUATION_QUADRATIC 0.1f

in v3 fP;
in v3 fN;
in v2 fUV;
in v4 fC;

out v4 result;

uniform v3 camera_pos;
uniform v3 directional_light_dir;
uniform v3 directional_light_color;

in v2 shadowmap_coord;
uniform sampler2D shadowmap;

void main()
{
    v3 color = pow(fC.rgb, v3(2.2));

    v3 to_cam = normalize(camera_pos - fP);
    v3 to_light = directional_light_dir;

    f32 shadowmap_value = texture(shadowmap, shadowmap_coord).r;

    color = pow(color, v3(1.0/2.2));
    result = v4(color, 1);
}

)";
