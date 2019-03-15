#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

typedef struct _tagRECT {
    int x;
    int y;
    int z;
    int o;
} RECT;

extern void DebugPrintRect(RECT);

static void test_main(void **state)
{
    DebugPrintRect((RECT){3, 3, 4, 5});
    (void) state; /* unused */
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_main),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
