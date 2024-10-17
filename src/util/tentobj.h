#ifndef TENTCORE_TENTOBJ_H
#define TENTCORE_TENTOBJ_H

#define DECLARE_$NEW_ARGS(T, ...)               \
T* T##_$new(__VA_ARGS__) {                      \
    void* pObject = malloc(sizeof(T));          \
    if (pObject) {                              \
        memset(pObject, 0, sizeof(T));          \
        T##_$init((T*)pObject, __VA_ARGS__);    \
    }                                           \
    return (T*)pObject;                         \
}

#define DECLARE_$NEW(T)                         \
T* T##_$new() {                                 \
    void* pObject = malloc(sizeof(T));          \
    if (pObject) {                              \
        memset(pObject, 0, sizeof(T));          \
        T##_$init((T*)pObject);                 \
    }                                           \
    return (T*)pObject;                         \
}

#endif  /* TENTCORE_TENTOBJ_H */
