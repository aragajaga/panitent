#ifndef PANITENT_BRUSH_H_
#define PANITENT_BRUSH_H_

#include "canvas.h"

typedef struct _Brush Brush;

Brush* Brush_Create(Canvas* tex);
void Brush_Draw(Brush* brush, int x, int y, Canvas* target);
void Brush_DrawTo(Brush* brush, int x0, int y0, int x1, int y1, Canvas* target);
void Brush_Delete(Brush* brush);

#endif  /* PANITENT_BRUSH_H_ */
