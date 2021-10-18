#ifndef PANITENT_TEST_H
#define PANITENT_TEST_H

#include <stdio.h>
#include <assert.h>

#define TEST(f, ...) \
{ \
  int ret = f(__VA_ARGS__); \
  assert(!ret); \
  result = result && ret; \
  printf("%s [%s]\n", #f, ret ? "FAILED" : "PASSED"); \
}

#define TEST_SUMMARY \
  printf("TEST SUMMARY [%s]\n", result ? "FAILED" : "PASSED"); \

#endif  /* PANITENT_TEST_H */
