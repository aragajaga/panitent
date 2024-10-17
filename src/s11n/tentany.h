#ifndef PANITENT_S11N_ANY_H
#define PANITENT_S11N_ANY_H

typedef enum {
    TENTANY_TYPE_EMPTY,
    TENTANY_TYPE_INT,
    TENTANY_TYPE_FLOAT,
    TENTANY_TYPE_DOUBLE,
    TENTANY_TYPE_STRING,
    TENTANY_TYPE_CUSTOM
} ETENTAnyType;

typedef struct TENTAny TENTAny;
typedef union TENTAnyUnion TENTAnyUnion;

struct TENTAny {
    ETENTAnyType type;
    union TENTAnyUnion {
        int intVal;
        float floatVal;
        double doubleVal;
        char* strVal;
        void* customVal;
    } data;
};

inline void TENTAny_Clear(TENTAny* pAny)
{
    switch (pAny->type)
    {
    case TENTANY_TYPE_STRING:
    case TENTANY_TYPE_CUSTOM:
        free(pAny->data.customVal);
        break;
    }

    memset(&pAny->data, 0, sizeof(TENTAnyUnion));
    pAny->type = TENTANY_TYPE_EMPTY;
}

inline void TENTAny_SetInt(TENTAny* pAny, int val)
{
    TENTAny_Clear(pAny);
    pAny->type = TENTANY_TYPE_INT;
    pAny->data.intVal = val;
}

inline void TENTAny_SetFloat(TENTAny* pAny, float val)
{
    TENTAny_Clear(pAny);
    pAny->type = TENTANY_TYPE_FLOAT;
    pAny->data.floatVal = val;
}

inline void TENTAny_SetDouble(TENTAny* pAny, double val)
{
    TENTAny_Clear(pAny);
    pAny->type = TENTANY_TYPE_DOUBLE;
    pAny->data.doubleVal = val;
}

inline void TENTAny_SetString(TENTAny* pAny, char* string)
{
    TENTAny_Clear(pAny);
    pAny->type = TENTANY_TYPE_STRING;
    pAny->data.strVal = _strdup(string);
}

inline void TENTAny_SetCustom(TENTAny* pAny, void* data, size_t size)
{
    TENTAny_Clear(pAny);
    pAny->type = TENTANY_TYPE_CUSTOM;
    void* pCustom = malloc(size);
    memcpy(pCustom, data, size);
    pAny->data.customVal = pCustom;
}

#endif  /* PANITENT_S11N_ANY_H */
