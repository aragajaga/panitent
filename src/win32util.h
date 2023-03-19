#ifndef WIN32UTIL_H
#define WIN32UTIL_H

#define HEX_TO_COLORREF(hexColor) ((COLORREF) ((hexColor & 0xFF) << 16) | (hexColor & 0xFF00) | ((hexColor & 0xFF0000) >> 16))

inline COLORREF HSVtoCOLORREF(float h, float s, float v)
{
    float c = v * s;
    float x = c * (1 - fabsf(fmodf(h / 60.0f, 2) - 1));
    float m = v - c;

    float r, g, b;

    if (h >= 0 && h < 60) {
        r = c;
        g = x;
        b = 0;
    }
    else if (h >= 60 && h < 120) {
        r = x;
        g = c;
        b = 0;
    }
    else if (h >= 120 && h < 180) {
        r = 0;
        g = c;
        b = x;
    }
    else if (h >= 180 && h < 240) {
        r = 0;
        g = x;
        b = c;
    }
    else if (h >= 240 && h < 300) {
        r = x;
        g = 0;
        b = c;
    }
    else {
        r = c;
        g = 0;
        b = x;
    }

    int red = (int)((r + m) * 255.0f);
    int green = (int)((g + m) * 255.0f);
    int blue = (int)((b + m) * 255.0f);

    return RGB(red, green, blue);
}

#endif  /* WIN32UTIL_H */
