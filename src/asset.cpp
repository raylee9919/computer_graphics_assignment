/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Sung Woo Lee $
   $Notice: (C) Copyright 2024 by Sung Woo Lee. All Rights Reserved. $
   ======================================================================== */

#pragma pack(push, 1)
struct BMP_Info_Header 
{
    u16 filetype;
    u32 filesize;
    u16 reserved1;
    u16 reserved2;
    u32 bitmap_offset;
    u32 bitmap_info_header_size;
    s32 width;
    s32 height;
    s16 plane;
    u16 bpp; // bits
    u32 compression;
    u32 image_size;
    u32 h_resolution;
    u32 v_resolution;
    u32 plt_entry_cnt;
    u32 important;

    u32 r_mask;
    u32 g_mask;
    u32 b_mask;
};

struct Bitmap 
{
    s32     width;
    s32     height;
    s32     pitch;
    u32     handle;
    size_t  size;
    void    *memory;
};

struct Bit_Scan_Result {
    b32 found;
    u32 index;
};
#pragma pack(pop)

struct Buffer 
{
    u32     size;
    void    *data;
};

static Bit_Scan_Result
find_least_significant_set_bit(u32 value) {
    Bit_Scan_Result result = {};

    for (u32 test = 0; test < 32; ++test) {
        if (value & (1 << test)) {
            result.index = test;
            result.found = true;
            break;
        }
    }

    return result;
}

static Buffer 
read_entire_file(const char *path)
{
    Buffer result = {};

    FILE *file = fopen(path, "rb");
    if (file)
    {
        fseek(file, 0, SEEK_END);
        size_t file_size = ftell(file);
        fseek(file, 0, SEEK_SET);

        result.data = (u8 *)malloc(file_size);
        result.size = (u32)file_size;

        fread(result.data, file_size, 1, file);
    }

    return result;
}

static Bitmap *
load_bmp(Memory_Arena *arena, const char *filename)
{
    Bitmap *result = push_struct(arena, Bitmap);
    *result = {};
    
    Buffer read = read_entire_file(filename);
    if (read.size != 0) 
    {
        BMP_Info_Header *header = (BMP_Info_Header *)read.data;
        u32 *pixels = (u32 *)((u8 *)read.data + header->bitmap_offset);

        result->memory  = pixels + header->width * (header->height - 1);
        result->width   = header->width;
        result->height  = header->height;
        result->pitch   = result->width * 4;
        result->size    = result->width * result->height * 4;
        result->handle  = 0;

        ASSERT(header->compression == 3);

        u32 r_mask = header->r_mask;
        u32 g_mask = header->g_mask;
        u32 b_mask = header->b_mask;
        u32 a_mask = ~(r_mask | g_mask | b_mask);        
        
        Bit_Scan_Result r_scan = find_least_significant_set_bit(r_mask);
        Bit_Scan_Result g_scan = find_least_significant_set_bit(g_mask);
        Bit_Scan_Result b_scan = find_least_significant_set_bit(b_mask);
        Bit_Scan_Result a_scan = find_least_significant_set_bit(a_mask);
        
        ASSERT(r_scan.found);
        ASSERT(g_scan.found);   
        ASSERT(b_scan.found);
        ASSERT(a_scan.found);

        s32 r_shift = (s32)r_scan.index;
        s32 g_shift = (s32)g_scan.index;
        s32 b_shift = (s32)b_scan.index;
        s32 a_shift = (s32)a_scan.index;

        f32 inv_255f = 1.0f / 255.0f;
        
        u32 *at = pixels;
        for(s32 y = 0;
            y < header->height;
            ++y)
        {
            for(s32 x = 0;
                x < header->width;
                ++x)
            {
                u32 c = *at;

                f32 r = (f32)((c & r_mask) >> r_shift);
                f32 g = (f32)((c & g_mask) >> g_shift);
                f32 b = (f32)((c & b_mask) >> b_shift);
                f32 a = (f32)((c & a_mask) >> a_shift);

                f32 ra = a * inv_255f;
#if 1
                r *= ra;
                g *= ra;
                b *= ra;
#endif

                *at++ = (((u32)(a + 0.5f) << 24) |
                         ((u32)(r + 0.5f) << 16) |
                         ((u32)(g + 0.5f) <<  8) |
                         ((u32)(b + 0.5f) <<  0));
            }
        }
        result->memory = pixels;
    }

    return result;
}

