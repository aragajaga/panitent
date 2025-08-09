#ifndef PANITENT_S11N_ANY_H
#define PANITENT_S11N_ANY_H

#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#ifdef _MSC_VER
#define TENTANY_STRDUP _strdup
#else
#define TENTANY_STRDUP strdup
#endif

typedef enum {
    TENTANY_TYPE_EMPTY = 0,
    TENTANY_TYPE_INT,
    TENTANY_TYPE_FLOAT,
    TENTANY_TYPE_DOUBLE,
    TENTANY_TYPE_STRING,
    TENTANY_TYPE_CUSTOM
} ETENTAnyType;

typedef struct TENTAny TENTAny;
typedef union TENTAnyUnion TENTAnyUnion;

typedef union TENTAnyUnion {
    int intVal;
    float floatVal;
    double doubleVal;
    char* strVal;
    void* customVal;
} TENTAnyUnion;

typedef struct TENTAny {
    ETENTAnyType type;
    TENTAnyUnion data;
    size_t customSize;
} TENTAny;

inline void TENTAny_Init(TENTAny* pAny)
{
    if (!pAny) return;
    pAny->type = TENTANY_TYPE_EMPTY;
    memset(&pAny->data, 0, sizeof(TENTAnyUnion));
    pAny->customSize = 0;
}

inline void TENTAny_Clear(TENTAny* pAny)
{
    if (!pAny)
    {
        return;
    }

    switch (pAny->type)
    {
    case TENTANY_TYPE_STRING:
        if (pAny->data.strVal)
        {
            free(pAny->data.strVal);
            pAny->data.strVal = NULL;
        }
        break;
    case TENTANY_TYPE_CUSTOM:
        if (pAny->data.customVal)
        {
            free(pAny->data.customVal);
            pAny->data.customVal = NULL;
        }
        pAny->customSize = 0;
        free(pAny->data.customVal);
        break;
    default:
        break;
    }

    memset(&pAny->data, 0, sizeof(TENTAnyUnion));
    pAny->type = TENTANY_TYPE_EMPTY;
    pAny->customSize = 0;
}

/* Deep-copy helper: copies contents from src int dst (dst is first cleared)
   Returns 1 on success, 0 on allocation failure or invalid args. */
inline int TENTAny_Copy(TENTAny* dst, const TENTAny* src)
{
    if (!dst || !src)
    {
        return 0;
    }
    TENTAny_Clear(dst);

    dst->type = src->type;
    dst->customSize = 0;

    switch (src->type)
    {
    case TENTANY_TYPE_INT:
        dst->data.intVal = src->data.intVal;
        break;
    case TENTANY_TYPE_FLOAT:
        dst->data.floatVal = src->data.floatVal;
        break;
    case TENTANY_TYPE_DOUBLE:
        dst->data.doubleVal = src->data.doubleVal;
        break;
    case TENTANY_TYPE_STRING:
        if (src->data.strVal)
        {
            dst->data.strVal = TENTANY_STRDUP(src->data.strVal);
            if (!dst->data.strVal)
            {
                dst->type = TENTANY_TYPE_EMPTY;
                return 0;   /* Allocation failed */
            }
        }
        else {
            dst->data.strVal = NULL;
        }
        break;
    case TENTANY_TYPE_CUSTOM:
        if (src->data.customVal && src->customSize > 0)
        {
            dst->data.customVal = malloc(src->customSize);
            if (!dst->data.customVal)
            {
                dst->type = TENTANY_TYPE_EMPTY;
                dst->customSize = 0;
                return 0;
            }
            memcpy(dst->data.customVal, src->data.customVal, src->customSize);
            dst->customSize = src->customSize;
        }
        else {
            /* Empty custom blob (NULL or size 0) */
            dst->data.customVal = NULL;
            dst->customSize = 0;
        }
        break;
    case TENTANY_TYPE_EMPTY:
    default:
        dst->type = TENTANY_TYPE_EMPTY;
        break;
    }

    return 1;
}

inline void TENTAny_SetInt(TENTAny* pAny, int val)
{
    if (!pAny) return;
    TENTAny_Clear(pAny);
    pAny->type = TENTANY_TYPE_INT;
    pAny->data.intVal = val;
}

inline void TENTAny_SetFloat(TENTAny* pAny, float val)
{
    if (!pAny) return;
    TENTAny_Clear(pAny);
    pAny->type = TENTANY_TYPE_FLOAT;
    pAny->data.floatVal = val;
}

inline void TENTAny_SetDouble(TENTAny* pAny, double val)
{
    if (!pAny) return;
    TENTAny_Clear(pAny);
    pAny->type = TENTANY_TYPE_DOUBLE;
    pAny->data.doubleVal = val;
}

/* Returns 1 on success, 0 on allocation failure */
inline int TENTAny_SetString(TENTAny* pAny, char* string)
{
    if (!pAny) return 0;
    TENTAny_Clear(pAny);
    pAny->type = TENTANY_TYPE_STRING;
    if (string)
    {
        pAny->data.strVal = TENTANY_STRDUP(string);
        if (!pAny->data.strVal)
        {
            pAny->type = TENTANY_TYPE_EMPTY;
            return 0;
        }
    }
    else {
        pAny->data.strVal = NULL;
    }
    return 1;
}

/* For custom data we need size so we can allocate and copy.
   Returns 1 on success, 0 on allocation failure. */
inline int TENTAny_SetCustom(TENTAny* pAny, void* data, size_t size)
{
    if (!pAny) return 0;
    TENTAny_Clear(pAny);
    pAny->type = TENTANY_TYPE_CUSTOM;
    pAny->customSize = 0;
    if (data && size > 0)
    {
        void* pCustom = malloc(size);
        if (!pCustom)
        {
            pAny->type = TENTANY_TYPE_EMPTY;
            return 0;
        }
        memcpy(pCustom, data, size);
        pAny->data.customVal = pCustom;
        pAny->customSize = size;
    }
    else {
        pAny->data.customVal = NULL;
        pAny->customSize = 0;
    }
    return 1;
}

#endif  /* PANITENT_S11N_ANY_H */
