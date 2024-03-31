#include "precomp.h"
#include "palette.h"

struct Palette {
    void* _stub;
};

static uint32_t defaultPaletteColors[] = {
    0xFF000000, /* Black */
    0xFFFFFFFF, /* White */
    0xFFCCCCCC, /* Light Gray */
    0xFF888888, /* Gray */
    0xFF444444, /* Dark Gray */

    0xFFFF8888, /* Light Red */
    0xFFFFFF88, /* Light Yellow */
    0xFF88FF88, /* Light Green */
    0xFF88FFFF, /* Light Cyan */
    0xFF8888FF, /* Light Blue */
    0xFFFF88FF, /* Light Magenta */

    0xFFFF0000, /* Red */
    0xFFFFFF00, /* Yellow */
    0xFF00FF00, /* Green */
    0xFF00FFFF, /* Cyan */
    0xFF0000FF, /* Blue */
    0xFFFF00FF, /* Magenta */

    0xFF880000, /* Crimson */
    0xFF888800, /* Gold */
    0xFF008800, /* Dark Green */
    0xFF008888, /* Dark Cyan */
    0xFF000088, /* Dark Blue */
    0xFF880088, /* Dark Magenta */

    0xCC000000, /* Black */
    0xCCFFFFFF, /* White */
    0xCCCCCCCC, /* Light Gray */
    0xCC888888, /* Gray */
    0xCC444444, /* Dark Gray */

    0xCCFF8888, /* Light Red */
    0xCCFFFF88, /* Light Yellow */
    0xCC88FF88, /* Light Green */
    0xCC88FFFF, /* Light Cyan */
    0xCC8888FF, /* Light Blue */
    0xCCFF88FF, /* Light Magenta */

    0xCCFF0000, /* Red */
    0xCCFFFF00, /* Yellow */
    0xCC00FF00, /* Green */
    0xCC00FFFF, /* Cyan */
    0xCC0000FF, /* Blue */
    0xCCFF00FF, /* Magenta */

    0xCC880000, /* Crimson */
    0xCC888800, /* Gold */
    0xCC008800, /* Dark Green */
    0xCC008888, /* Dark Cyan */
    0xCC000088, /* Dark Blue */
    0xCC880088, /* Dark Magenta */

    0x88000000, /* Black */
    0x88FFFFFF, /* White */
    0x88CCCCCC, /* Light Gray */
    0x88888888, /* Gray */
    0x88444444, /* Dark Gray */

    0x88FF8888, /* Light Red */
    0x88FFFF88, /* Light Yellow */
    0x8888FF88, /* Light Green */
    0x8888FFFF, /* Light Cyan */
    0x888888FF, /* Light Blue */
    0x88FF88FF, /* Light Magenta */

    0x88FF0000, /* Red */
    0x88FFFF00, /* Yellow */
    0x8800FF00, /* Green */
    0x8800FFFF, /* Cyan */
    0x880000FF, /* Blue */
    0x88FF00FF, /* Magenta */

    0x88880000, /* Crimson */
    0x88888800, /* Gold */
    0x88008800, /* Dark Green */
    0x88008888, /* Dark Cyan */
    0x88000088, /* Dark Blue */
    0x88880088, /* Dark Magenta */
};

void Palette_InitDefault(Palette*);

Palette* Palette_Create()
{
  Palette* palette = calloc(1, sizeof(Palette));
  if (palette)
  {
      kv_init(*palette);
      Palette_InitDefault(palette);
      return palette;
  }
  
  return NULL;
}

size_t Palette_GetSize(Palette* palette)
{
    if (palette)
    {
        return kv_size(*palette);
    }

    return 0;
}

uint32_t Palette_At(Palette* palette, size_t idx)
{
    if (palette)
    {
        return kv_a(uint32_t, *palette, idx);
    }

    return 0;
}

void Palette_Add(Palette* palette, uint32_t color)
{
    if (palette)
    {
        kv_push(uint32_t, *palette, color);
    }
}

void Palette_InitDefault(Palette* palette)
{
  for (size_t i = 0; i < ARRAYSIZE(defaultPaletteColors); ++i)
  {
    Palette_Add(palette, defaultPaletteColors[i]);
  }
}
