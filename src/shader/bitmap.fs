R"(
uniform sampler2D texture_sample;

in v2 fUV;
out v4 C;

void main()
{
    // sRGB
    v4 texture_color = texture(texture_sample, fUV);
    C = texture_color;
    C.rgb = pow(texture_color.rgb, v3(2.2));

    // sRGB
    if (C.a == 0.0f) {
        discard;
    } else {
        C.rgb = pow(C.rgb, v3(1.0/2.2));
    }
}

)";
