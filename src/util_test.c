#include "util.h"
#include "test.h"

#include <string.h>
#include <time.h>
#include <stdlib.h>

int test_vector()
{
    typedef PNTVECTOR(int) int_vec_t;

    int_vec_t intVec;
    memset(&intVec, 0, sizeof(int_vec_t));

    PNTVECTOR_init(&intVec);
    assert(intVec.capacity == DEFAULT_CAPACITY);
    assert(intVec.size == 0);
    assert(intVec.data != NULL);

    int values[64];
    memset(values, 0, sizeof(values));

    srand(time(NULL));

    for (size_t i = 0; i < ARRLEN(values); ++i)
        values[i] = rand();

    for (size_t i = 0; i < ARRLEN(values); ++i)
    {
        PNTVECTOR_add(&intVec, (values[i]));

        assert(intVec.size <= intVec.capacity);

        if (!(i % DEFAULT_CAPACITY))
            assert(intVec.capacity == _msize(intVec.data) /
                sizeof(typeof(intVec.data[0])));
    }

    assert(ARRLEN(values) == intVec.size);

    int eq = 1;
    for (size_t i = 0; i < intVec.size; ++i) {
        eq = eq && (intVec.data[i] == values[i]);
    }

    return !eq;
}

int test_map()
{
    typedef PNTMAP(char, unsigned int) test_map_t;

    test_map_t map;
    memset(&map, 0, sizeof(test_map_t));

    PNTMAP_init(&map);
    assert(map.capacity == DEFAULT_CAPACITY);
    assert(map.size == 0);
    assert(map.data != NULL);

    struct {
        char i1;
        unsigned int i2;
    } values[] = {
      {'Y', 1337u},
      {'z', 3221u},
      {'X', 9933u},
      {'u', 4323u},
      {'s', 5324u},
      {'c', 8303u},
    };

    for (size_t i = 0; i < ARRLEN(values); ++i)
    {
        PNTMAP_add_pair(&map, values[i].i1, values[i].i2);
    }

    assert(map.size == ARRLEN(values));

    int eq = 1;
    for (size_t i = 0; i < map.size; ++i)
    {
        eq = eq && (
            (map.data[i].item1 == values[i].i1) &&
            (map.data[i].item2 == values[i].i2));
    }

    return !eq;
}

int main()
{
    int result;
    TEST(test_vector);
    TEST(test_map);
    TEST_SUMMARY

        return result;
}
