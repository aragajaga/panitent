#include "precomp.h"

#include <stdio.h>

#include "debug.h"

void DebugPrintRect(RECT* rc)
{
  printf("\tright:\t%ld\n"
         "\ttop:\t%ld\n"
         "\tleft:\t%ld\n"
         "\tbottom:\t%ld\n",
         rc->right,
         rc->top,
         rc->left,
         rc->bottom);
}
