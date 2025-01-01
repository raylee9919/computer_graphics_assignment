R"(

uniform m4x4  M;
uniform m4x4  VP;

layout (location = 0) in v3 vP;
layout (location = 1) in v3 vN;
layout (location = 2) in v2 vUV;
layout (location = 3) in v4 vC;

out v3 fP;
out v3 fN;
out v2 fUV;
out v4 fC;

void main()
{
    m3x3 orientation = m3x3(M);

    fN = normalize(orientation * vN);

    v4 result_pos = M * v4(vP, 1.0f);
    fP = result_pos.xyz;
    fUV = vUV;
    fC  = vC;

    gl_Position = VP * result_pos;
}

)";
