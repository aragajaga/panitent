#include "../precomp.h"

#include "jsonserializer.h"

#include <stdlib.h>
#include <string.h>
#include <inttypes.h>   /* for portable integer macros if needed */
#include <stdbool.h>

/* Forward declarations */
void __impl_JSONSerializer_Serialize(JSONSerializer* pSerializer, PropertyTreeNode* pPropTree);

/* vtable for ISerializer interface */
JSONSerializer_vtbl __vtbl_JSONSerializer = {
    .Serialize = __impl_JSONSerializer_Serialize
};

void JSONSerializer_Init(JSONSerializer* pSerializer)
{
    pSerializer->pVtbl = &__vtbl_JSONSerializer;
}

void JSONEscapeAndPrintf(FILE* out, const char* s)
{
    if (!out) out = stdout;
    if (!s) return;

    for (const unsigned char* p = (const unsigned char*)s; *p; ++p)
    {
        unsigned char c = *p;
        switch (c)
        {
        case '\"': fputs("\\\"", out); break;
        case '\\': fputs("\\\\", out); break;
        case '\b': fputs("\\b", out); break;
        case '\f': fputs("\\f", out); break;
        case '\n': fputs("\\n", out); break;
        case '\r': fputs("\\r", out); break;
        case '\t': fputs("\\t", out); break;
        default:
            if (c < 0x20)
            {
                /* control character -> \u00XX */
                fprintf(out, "\\u%04x", c);
            }
            else {
                fputc(c, out);
            }
            break;
        }
    }
}

/* Print a TENTAny value as JSON (value only; caller handles commas/keys). */
void JSONPrintValue(FILE* out, const TENTAny* val)
{
    if (!out || !val)
    {
        fputs("null", stdout);
        return;
    }

    switch (val->type)
    {
    case TENTANY_TYPE_EMPTY:
        fputs("null", out);
        break;
    case TENTANY_TYPE_INT:
        fprintf(out, "%d", val->data.intVal);
        break;
    case TENTANY_TYPE_FLOAT:
        /* using genering %g for compact formatting */
        fprintf(out, "%g", (double)val->data.floatVal);
        break;
    case TENTANY_TYPE_STRING:
        if (val->data.strVal)
        {
            fputc('"', out);
            JSONEscapeAndPrintf(out, val->data.strVal);
            fputc('"', out);
        }
        else {
            fputs("null", out);
        }
        break;
    case TENTANY_TYPE_CUSTOM:
        /* Custom binary blob: we don't attempt full encoding here.
           We print an object with size and a placeholder */
        fprintf(out, "{\"__custom_size\": %zu, \"__custom\": \"<binary>\"}", val->customSize);
        break;
    default:
        fputs("null", out);
        break;
    }
}

/* Recursively serialize a set of sibling nodes as a JSON object.
   - node: first node in sibling list
   - out: FILE* to write to
   This writes: { "key1": value1, "key2": value2, ... }   
*/
void JSONSerializeNodeObject(FILE* out, PropertyTreeNode* node)
{
    if (!out) out = stdout;
    fputc('{', out);

    bool first = true;
    for (PropertyTreeNode* cur = node; cur != NULL; cur = cur->sibling)
    {
        if (!cur->key) continue;

        if (!first) fputc(',', out);
        first = false;

        /* key */
        fputc('"', out);
        JSONEscapeAndPrintf(out, cur->key);
        fputc('"', out);
        fputc(':', out);

        /* value or children */
        if (cur->child)
        {
            /* Node has children -> serialize children as nested object */
            /* [WARNING] TODO: Recursion is BAD and can cause stack overflow. Use heap-allocated stack data structure */
            JSONSerializeNodeObject(out, cur->child);

            /* If the node also has a non-empty value, include it as "__value" */
            if (cur->value.type != TENTANY_TYPE_EMPTY)
            {
                fputc(',', out);
                fputs("\"__value\":", out);
                JSONPrintValue(out, &cur->value);
            }
        }
        else {
            /* Leaf: print the stored value */
            JSONPrintValue(out, &cur->value);
        }
    }

    fputc('}', out);
}

void __impl_JSONSerializer_Serialize(JSONSerializer* pSerializer, PropertyTreeNode* pPropTree)
{
    (void)pSerializer;  /* unused for now */

    if (!pPropTree)
    {
        /* nothing to print */
        puts("null");
        return;
    }

    /* If pPropTree is a list of siblings at top-level, produce an object with all top-level keys. */
    JSONSerializeNodeObject(stdout, pPropTree);
    fputc('\n', stdout);
}
