#ifndef PANITENT_GRIMSTROKE_MSAA_H
#define PANITENT_GRIMSTROKE_MSAA_H

#include "../util/tenttypes.h"

typedef struct GrimstrokeMSAA GrimstrokeMSAA;
struct GrimstrokeMSAA {
    unsigned int wSamples;
    unsigned int hSamples;
    unsigned long long* bitMask;
};

#endif  /* PANITENT_GRIMSTROKE_MSAA_H */
