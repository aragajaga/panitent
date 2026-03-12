#pragma once

#include <stdio.h>
#ifdef _WIN32
#include <windows.h>
#endif

#define ASSERT(condition) \
    do { \
        if (!(condition)) { \
            fprintf(stderr, "Assertion failed: %s, file %s, line %d\n", #condition, __FILE__, __LINE__); \
            DebugBreak(); \
        } \
    } while(0)
