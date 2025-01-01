/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Sung Woo Lee $
   $Notice: (C) Copyright 2024 by Sung Woo Lee. All Rights Reserved. $
   ======================================================================== */

#define pi32                3.14159265359f
#define epsilon_f32         1.19209e-07f

//
// @trigonometry
//
inline f32
cos(f32 x) 
{
    f32 result = _mm_cvtss_f32(_mm_cos_ps(_mm_set1_ps(x)));
    return result;
}

inline f32
acos(f32 x) 
{
    f32 result = _mm_cvtss_f32(_mm_acos_ps(_mm_set1_ps(x)));
    return result;
}

inline f32
sin(f32 x) 
{
    f32 result = _mm_cvtss_f32(_mm_sin_ps(_mm_set1_ps(x)));
    return result;
}

inline f32
tan(f32 x) 
{
    f32 result = _mm_cvtss_f32(_mm_tan_ps(_mm_set1_ps(x)));
    return result;
}

//
// @convert
//
inline f32
sqrt(f32 x) {
    f32 result = _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(x)));
    return result;
}

inline s32 
round_f32_to_s32(f32 x) {
    s32 result = _mm_cvtss_si32(_mm_set_ss(x));
    return result;
}

inline u32 
round_f32_to_u32(f32 x) {
    s32 result = (u32)_mm_cvtss_si32(_mm_set_ss(x));
    return result;
}

inline s32
floor_f32_to_s32(f32 A) {
    s32 result = (s32)A;
    return result;
}

inline s32
ceil_f32_to_s32(f32 A) {
    s32 result = (s32)A + 1;
    return result;
}

inline f32
square(f32 val) 
{
    f32 result = val * val;
    return result;
}

//
// v2
//
union v2 {
    struct { f32 x, y; };
    f32 e[2];
};

inline v2
operator - (const v2 &in) 
{
    v2 V;
    V.x = -in.x;
    V.y = -in.y;
    return V;
}

inline v2
operator * (f32 A, v2 B) 
{
    v2 result;
    result.x = A * B.x;
    result.y = A * B.y;

    return result;
}

inline v2
operator * (v2 B, f32 A) 
{
    v2 result;
    result.x = A * B.x;
    result.y = A * B.y;

    return result;
}

inline v2
operator + (v2 A, v2 B) 
{
    v2 result;
    result.x = A.x + B.x;
    result.y = A.y + B.y;

    return result;
}

inline v2
operator - (v2 A, v2 B) 
{
    v2 result;
    result.x = A.x - B.x;
    result.y = A.y - B.y;

    return result;
}

inline v2&
operator += (v2& a, v2 b) 
{
    a.x += b.x;
    a.y += b.y;

    return a;
}

inline v2&
operator -= (v2& a, v2 b) 
{
    a.x -= b.x;
    a.y -= b.y;

    return a;
}

inline v2&
operator *= (v2& a, f32 b) 
{
    a.x *= b;
    a.y *= b;

    return a;
}

inline f32
dot(v2 a, v2 b) 
{
    f32 result = a.x * b.x + a.y * b.y;
    return result;
}

inline v2
hadamard(v2 A, v2 B) 
{
    v2 result = {
        A.x * B.x,
        A.y * B.y,
    };

    return result;
}

inline f32
length_square(v2 A) 
{
    f32 result = dot(A, A);
    return result;
}

inline f32
inv_length_square(v2 A) 
{
    f32 result = 1.0f / dot(A, A);
    return result;
}

inline f32
len(v2 A) 
{
    f32 result = sqrt(length_square(A));
    return result;
}

inline v2
normalize(v2 a) 
{
    v2 r = a;
    f32 inv_len = (1.0f / len(r));
    r *= inv_len;
    return r;
}

//
// v3
//
union v3 
{
    struct { f32 x, y, z; };
    struct { v2 xy; f32 z; };
    f32 e[3];
};

inline v3
V3(f32 x, f32 y, f32 z)
{
    v3 v = v3{x,y,z};
    return v;
}

inline v3
V3(f32 a)
{
    v3 v = v3{a,a,a};
    return v;
}

inline v3
operator - (const v3 &in) 
{
    v3 V;
    V.x = -in.x;
    V.y = -in.y;
    V.z = -in.z;
    return V;
}

inline v3
operator * (f32 A, v3 B) 
{
    v3 result;
    result.x = A * B.x;
    result.y = A * B.y;
    result.z = A * B.z;

    return result;
}

inline v3
operator * (v3 B, f32 A) 
{
    v3 result;
    result.x = A * B.x;
    result.y = A * B.y;
    result.z = A * B.z;

    return result;
}

inline v3
operator + (v3 A, v3 B) 
{
    v3 result;
    result.x = A.x + B.x;
    result.y = A.y + B.y;
    result.z = A.z + B.z;

    return result;
}

inline v3
operator - (v3 A, v3 B) 
{
    v3 result;
    result.x = A.x - B.x;
    result.y = A.y - B.y;
    result.z = A.z - B.z;

    return result;
}

inline v3&
operator += (v3& a, v3 b) 
{
    a.x += b.x;
    a.y += b.y;
    a.z += b.z;

    return a;
}

inline v3&
operator -= (v3& a, v3 b) 
{
    a.x -= b.x;
    a.y -= b.y;
    a.z -= b.z;

    return a;
}

inline v3&
operator *= (v3& a, f32 b) 
{
    a.x *= b;
    a.y *= b;
    a.z *= b;

    return a;
}

inline f32
dot(v3 a, v3 b) 
{
    f32 result = (a.x * b.x +
                  a.y * b.y +
                  a.z * b.z);

    return result;
}

inline f32
length_square(v3 A) 
{
    f32 result = dot(A, A);
    return result;
}

inline f32
len(v3 A) 
{
    f32 result = sqrt(length_square(A));
    return result;
}

inline v3
normalize(v3 a) 
{
    v3 r = a;
    f32 inv_len = (1.0f / len(r));
    r *= inv_len;
    return r;
}

inline v3
hadamard(v3 A, v3 B) 
{
    v3 result = {
        A.x * B.x,
        A.y * B.y,
        A.z * B.z 
    };
    return result;
}

inline v3
cross(v3 A, v3 B) 
{
    v3 R = {};
    R.x = (A.y * B.z) - (B.y * A.z);
    R.y = (A.z * B.x) - (B.z * A.x);
    R.z = (A.x * B.y) - (B.x * A.y);
    return R;
}

inline v3
noz(v3 a)
{
    v3 result = {};

    f32 lensq = length_square(a);
    if (lensq > square(0.0001f))
        result = (1.0f / sqrt(lensq)) * a;
    
    return result;
}

//
// v4
//
union v4 {
    struct {
        union {
            struct { f32 r, g, b; };
            v3 rgb;
        };
        f32 a;
    };
    struct {
        union {
            struct { f32 x, y, z; };
            v3 xyz;
        };
        f32 w;
    };
    f32 e[4];
};

inline v4
V4(f32 r, f32 g, f32 b, f32 a)
{
    v4 V;
    V.r = r;
    V.g = g;
    V.b = b;
    V.a = a;
    return V;
}

inline v4
V4(v3 a, f32 b)
{
    v4 V;
    V.x = a.x;
    V.y = a.y;
    V.z = a.z;
    V.w = b;
    return V;
}


//
// quaternion
//
struct qt {
    f32 w, x, y, z;
};

inline qt
operator + (qt a, qt b)
{
    qt q = {};
    q.w = a.w + b.w;
    q.x = a.x + b.x;
    q.y = a.y + b.y;
    q.z = a.z + b.z;

    return q;
}

inline qt
operator * (qt a, qt b)
{
    qt q = {};
    q.w = (a.w * b.w) - (a.x * b.x) - (a.y * b.y) - (a.z * b.z); 
    q.x = (a.w * b.x) + (a.x * b.w) + (a.y * b.z) - (a.z * b.y); 
    q.y = (a.w * b.y) + (a.y * b.w) + (a.z * b.x) - (a.x * b.z); 
    q.z = (a.w * b.z) + (a.z * b.w) + (a.x * b.y) - (a.y * b.x); 

    return q;
}

inline qt
operator * (qt a, f32 b)
{
    qt q = {};
    q.w = a.w * b;
    q.x = a.x * b;
    q.y = a.y * b;
    q.z = a.z * b;

    return q;
}

inline qt
operator * (f32 b, qt a)
{
    qt q = {};
    q.w = a.w * b;
    q.x = a.x * b;
    q.y = a.y * b;
    q.z = a.z * b;

    return q;
}

inline qt
operator - (qt in)
{
    qt q = {};
    q.w = -in.w;
    q.x = -in.x;
    q.y = -in.y;
    q.z = -in.z;

    return q;
}

inline f32
dot(qt a, qt b)
{
    f32 result = ( (a.w * b.w) +
                   (a.x * b.x) +
                   (a.y * b.y) +
                   (a.z * b.y) );
    return result;
}

inline qt 
rotate(qt q, v3 axis, f32 t)
{
    f32 c = cos(t * 0.5f);
    f32 s = sin(t * 0.5f);
    v3 n = s * normalize(axis);
    qt result = qt{c, n.x, n.y, n.z}* q;
    return result;
}

//
// m4x4
//

// @Important: row-major!
struct m4x4 {
    f32 e[4][4];
};

inline m4x4
operator * (m4x4 a, m4x4 b) 
{
    m4x4 R = {};

    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4; ++c) {
            for (int i = 0; i < 4; ++i) {
                R.e[r][c] += a.e[r][i] * b.e[i][c];
            }
        }
    }

    return R;
}

inline v4
operator * (m4x4 m, v4 p)
{
    v4 res = v4{};
    for (int i = 0 ; i < 4; ++i) {
        for (int j = 0 ; j < 4; ++j) { 
            res.e[i] += (m.e[i][j] * p.e[j]);
        }
    }
    return res;
}

static v3
transform(m4x4 M, v3 P, f32 Pw = 1.0f) 
{
    v3 R;

    R.x = (M.e[0][0] * P.x +
           M.e[0][1] * P.y +
           M.e[0][2] * P.z +
           M.e[0][3] * Pw);

    R.y = (M.e[1][0] * P.x +
           M.e[1][1] * P.y +
           M.e[1][2] * P.z +
           M.e[1][3] * Pw);

    R.z = (M.e[2][0] * P.x +
           M.e[2][1] * P.y +
           M.e[2][2] * P.z +
           M.e[2][3] * Pw);

    return R;
}

inline v3
operator * (m4x4 m, v3 p) 
{
    v3 r = transform(m, p);
    return r;
}

inline m4x4
identity() 
{
    m4x4 r = {
        {{ 1,  0,  0,  0 },
         { 0,  1,  0,  0 },
         { 0,  0,  1,  0 },
         { 0,  0,  0,  1 }},
    };
    return r;
}

inline m4x4
x_rotation(f32 a) 
{
    f32 c = cos(a);
    f32 s = sin(a);
    m4x4 r = {
        {{ 1,  0,  0,  0 },
         { 0,  c, -s,  0 },
         { 0,  s,  c,  0 },
         { 0,  0,  0,  1 }},
    };

    return r;
}

inline m4x4
y_rotation(f32 a) 
{
    f32 c = cos(a);
    f32 s = sin(a);
    m4x4 r = {
        {{ c,  0,  s,  0 },
         { 0,  1,  0,  0 },
         {-s,  0,  c,  0 },
         { 0,  0,  0,  1 }},
    };

    return r;
}

inline m4x4
z_rotation(f32 a) 
{
    f32 c = cos(a);
    f32 s = sin(a);
    m4x4 r = {
        {{ c, -s,  0,  0 },
         { s,  c,  0,  0 },
         { 0,  0,  1,  0 },
         { 0,  0,  0,  1 }}
    };

    return r;
}

inline m4x4
transpose(m4x4 m) 
{
    m4x4 r = {};
    for (int i = 0; i < 4; ++i) 
        for (int j = 0; j < 4; ++j) 
            r.e[j][i] = m.e[i][j];
    return r;
}

inline m4x4
rows(v3 x, v3 y, v3 z) 
{
    m4x4 r = {
        {{ x.x, x.y, x.z,  0 },
         { y.x, y.y, y.z,  0 },
         { z.x, z.y, z.z,  0 },
         {   0,   0,   0,  1 }}
    };
    return r;
}

inline m4x4
columns(v3 x, v3 y, v3 z) 
{
    m4x4 r = {
        {{ x.x, y.x, z.x,  0 },
         { x.y, y.y, z.y,  0 },
         { x.z, y.z, z.z,  0 },
         {   0,   0,   0,  1 }}
    };
    return r;
}

static m4x4
translate(m4x4 m, v3 t) 
{
    m4x4 result = m;
    result.e[0][3] += t.x;
    result.e[1][3] += t.y;
    result.e[2][3] += t.z;

    return result;
}

static m4x4
camera_transform(v3 x, v3 y, v3 z, v3 p) 
{
    m4x4 R = rows(x, y, z);
    R = translate(R, -(R * p));

    return R;
}

inline v3
get_row(m4x4 M, u32 R) 
{
    v3 V = {
        M.e[R][0],
        M.e[R][1],
        M.e[R][2]
    };
    return V;
}

inline v3
get_column(m4x4 M, u32 C) 
{
    v3 V = {
        M.e[0][C],
        M.e[1][C],
        M.e[2][C]
    };
    return V;
}

inline f32
safe_ratio(f32 a, f32 b)
{
    return b == 0 ? 0 : a / b;
}

static m4x4
to_m4x4(qt q) 
{
    m4x4 result = identity();
    f32 w = q.w;
    f32 x = q.x;
    f32 y = q.y;
    f32 z = q.z;

    result.e[0][0] = 1.0f - 2.0f * (y * y + z * z);
    result.e[0][1] = 2.0f * (x * y - z * w);
    result.e[0][2] = 2.0f * (x * z + y * w);

    result.e[1][0] = 2.0f * (x * y + z * w);
    result.e[1][1] = 1.0f - 2.0f * (x * x + z * z);
    result.e[1][2] = 2.0f * (y * z - x * w);

    result.e[2][0] = 2.0f * (x * z - y * w);
    result.e[2][1] = 2.0f * (y * z + x * w);
    result.e[2][2] = 1.0f - 2.0f * (x * x + y * y);

    return result;
}

static m4x4
scale(m4x4 m, v3 s) 
{
    m4x4 r = m;
    r.e[0][0] *= s.x;
    r.e[1][1] *= s.y;
    r.e[2][2] *= s.z;

    return r;
}

inline m4x4
to_transform(v3 position, qt orientation, v3 scaling)
{
    m4x4 T = translate(identity(), position);
    m4x4 R = to_m4x4(orientation);
    m4x4 S = scale(identity(), scaling);
    m4x4 result = T*R*S;
    return result;
}

inline v3
rotate(v3 a, qt b)
{
    v3 result = (to_m4x4(b) * V4(a,1)).xyz;
    return result;
}

//
// AABB
//
struct AABB
{
    v3 cen;
    v3 dim;
};

static b32
is_in(v3 p, AABB aabb)
{
    v3 aabb_min = aabb.cen - aabb.dim * 0.5f - V3(epsilon_f32);
    v3 aabb_max = aabb.cen + aabb.dim * 0.5f + V3(epsilon_f32);
    b32 result = (p.x >= aabb_min.x && p.x <= aabb_max.x &&
                  p.y >= aabb_min.y && p.y <= aabb_max.y &&
                  p.z >= aabb_min.z && p.z <= aabb_max.z);
    return result;
}
