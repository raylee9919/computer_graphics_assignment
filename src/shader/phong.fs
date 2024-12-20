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
    v3 to_cam = normalize(camera_pos - fP);
    v3 dir = normalize(point_light_pos - fP);
    v3 reflect_dir = reflect(dir, fN);
    f32 cos_falloff = max(0, dot(dir, fN));
    f32 specular = pow(max(0.0, dot(to_cam, reflect_dir)), 32);

    f32 ambient = 0.1;
    f32 diffuse_str = 0.5f;
    f32 specular_str = 0.5f;

    v3 ambient_light = point_light_color * ambient;
    v3 diffuse_light = point_light_color * attenuation * diffuse_str * cos_falloff;
    v3 specular_light = point_light_color * attenuation * specular_str * specular;

    v3 light = ambient_light + diffuse_light + specular_light;
    color *= light;

    color = pow(color, v3(1.0/2.2));
    result = v4(color, 1);
}

)";
