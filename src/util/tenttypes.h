#ifndef PANITENT_UTIL_TENTTYPES_H
#define PANITENT_UTIL_TENTTYPES_H

#define __TENT_DECLARE_VEC2T(T, TN) \
typedef struct TENTVec2##TN { \
    union { \
        T x; \
        T w; \
        T width; \
    }; \
    union { \
        T y; \
        T h; \
        T height; \
    }; \
} TENTVec2##TN;

#define __TENT_DECLARE_VEC3T(T, TN) \
typedef struct TENTVec3##TN { \
    union { \
        T x; \
        T w; \
        T width; \
    }; \
    union { \
        T y; \
        T h; \
        T height; \
    }; \
    union { \
        T z; \
        T d; \
        T depth; \
    }; \
} TENTVec3##TN;

__TENT_DECLARE_VEC2T(int, i)
__TENT_DECLARE_VEC2T(unsigned, u)
__TENT_DECLARE_VEC2T(float, f)
__TENT_DECLARE_VEC2T(double, d)

__TENT_DECLARE_VEC3T(int, i)
__TENT_DECLARE_VEC3T(unsigned, u)
__TENT_DECLARE_VEC3T(float, f)
__TENT_DECLARE_VEC3T(double, d)

typedef TENTVec2i TENTSizei, TENTPointi, TENTPosi;
typedef TENTVec2u TENTSizeu, TENTPointu, TENTPosu;
typedef TENTVec2f TENTSizef, TENTPointf, TENTPosf;

#endif  /* PANITENT_UTIL_TENTTYPES_H */
