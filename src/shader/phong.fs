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
uniform v3 point_light_pos;
uniform v3 point_light_color;

void main()
{
    v3 color = pow(fC.rgb, v3(2.2));
    f32 d = distance(fP, point_light_pos);
    f32 attenuation = 1.0f / (LIGHT_ATTENUATION_CONSTANT +
                              LIGHT_ATTENUATION_LINEAR * d +
                              LIGHT_ATTENUATION_QUADRATIC * d * d);
    v3 light = point_light_color * attenuation;
    color *= light;
    color = pow(color, v3(1.0/2.2));
    result = v4(color * attenuation, 1);
}

)";
