#ifndef PANITENT_CHECKER_H_
#define PANITENT_CHECKER_H_

typedef struct _CheckerConfig {
  unsigned int tileSize; 
  uint32_t color[2];
} CheckerConfig;

HBRUSH CreateChecker(CheckerConfig* config, HDC hdc);

#endif  /* PANITENT_CHECKER_H_ */
