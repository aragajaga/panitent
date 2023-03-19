#ifndef PANITENT_PALETTE_SETTINGS_DLG_H_
#define PANITENT_PALETTE_SETTINGS_DLG_H_

#define IDM_PALETTESETTINGS 101

struct _PaletteSettings {
  int swatchSize;
  int checkerSize;
  uint32_t checkerColor1;
  uint32_t checkerColor2;
  HBRUSH checkerBrush;
  BOOL checkerInvalidate;
};

extern struct _PaletteSettings g_paletteSettings;

#endif  /* PANITENT_PALETTE_SETTINGS_DLG_H_ */
