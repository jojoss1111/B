#ifndef FB_3D_MAT_HPP
#define FB_3D_MAT_HPP

#include "fb_3d.hpp"

namespace mat3d {

inline mat3 rot_x(uint16_t a) {
    const int32_t c = trig::cosseno(a), s = trig::seno(a);
    return {{{trig::UM, 0, 0}, {0, c, -s}, {0, s, c}}};
}

inline mat3 rot_y(uint16_t a) {
    const int32_t c = trig::cosseno(a), s = trig::seno(a);
    return {{{c, 0, s}, {0, trig::UM, 0}, {-s, 0, c}}};
}

inline mat3 rot_z(uint16_t a) {
    const int32_t c = trig::cosseno(a), s = trig::seno(a);
    return {{{c, -s, 0}, {s, c, 0}, {0, 0, trig::UM}}};
}

inline mat3 mul(const mat3& a, const mat3& b) {
    mat3 r;
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++) {
            int64_t s = 0;
            for (int k = 0; k < 3; k++) s += (int64_t)a.m[i][k] * b.m[k][j];
            r.m[i][j] = (int32_t)(s >> trig::BITS);
        }
    return r;
}

inline mat3 modelo(const Form3d& f) {
    return mul(rot_y(f.rot_y()), mul(rot_x(f.rot_x()), rot_z(f.rot_z())));
}

}

#endif
