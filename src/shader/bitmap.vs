R"(
v2 UV[4] = v2[](v2(0, 1), v2(0, 0), v2(1, 1), v2(1, 0));
v2 data[4] = v2[](v2(0, 0), v2(0, -1), v2(1, 0), v2(1, -1));

out v2 fUV;

void main()
{
    fUV = UV[gl_VertexID];
    gl_Position = v4(data[gl_VertexID], -1.0, 1.0);
}

)";
