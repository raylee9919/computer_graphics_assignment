/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Sung Woo Lee $
   $Notice: (C) Copyright 2024 by Sung Woo Lee. All Rights Reserved. $
   ======================================================================== */



    

#if 0 // @NOTE: busted asf.
                    f32 t_remaining = dt * speed;
                    v3 original_pos = camera.position;
                    v3 cur_pos = camera.position;
                    v3 next_pos = cur_pos + t_remaining * dir;
                    for (u32 test = 0;
                         test < 4 && t_remaining > 0.001f;
                         ++test)
                    {
                        for (u32 idx = 0; idx < array_count(walls); ++idx)
                        {
                            Wall wall = walls[idx];
                            AABB aabb = wall.collision_volume;
                            aabb.dim += camera.collision_volume.dim;
                            if (is_in(next_pos, aabb)) 
                            {
                                v3 A[] = {
                                    aabb.cen - aabb.dim * 0.5f, aabb.cen - aabb.dim * 0.5f, aabb.cen - aabb.dim * 0.5f,
                                    aabb.cen + aabb.dim * 0.5f, aabb.cen + aabb.dim * 0.5f, aabb.cen + aabb.dim * 0.5f,
                                };
                                v3 N[] = {
                                    v3{ 0,-1, 0}, v3{-1, 0, 0}, v3{ 0, 0,-1},
                                    v3{ 0, 1, 0}, v3{ 1, 0, 0}, v3{ 0, 0, 1},
                                };

                                f32 t = FLT_MAX;
                                s32 normal_idx = -1;
                                for (s32 i = 0; i < array_count(N); ++i)
                                {
                                    f32 new_t = safe_ratio( dot(A[i] - cur_pos, N[i]), dot(dir, N[i]) );
                                    v3 collision_pos = cur_pos + new_t * dir;
                                    if (is_in(collision_pos, aabb))
                                    {
                                        t = MIN(t, new_t);
                                        normal_idx = i;
                                    }
                                }
                                ASSERT(t != FLT_MAX);
                                t -= 0.001f;
                                t_remaining -= t;
                                ASSERT(t_remaining >= 0.0f);
                                ASSERT(normal_idx != -1);

                                v3 n = N[normal_idx];
                                v3 tmp = (t - dt * speed) * dir;
                                v3 reflect = -tmp + 2.0f * dot(n, tmp) * n;
                                cur_pos += dir * t;
                                next_pos = cur_pos + reflect;
                            }
                        }
                    }
                    camera.position = next_pos;
#else
