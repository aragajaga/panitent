cmake_minimum_required(VERSION 3.17)
project(checker)

set(SOURCES
  src/main.c
  src/swatch.c
  res/checker.rc
)

set(EXECUTABLE_NAME ${PROJECT_NAME})

add_executable(${EXECUTABLE_NAME} WIN32 ${SOURCES})
target_precompile_headers(${EXECUTABLE_NAME} PUBLIC src/precomp.h)
target_link_libraries(${EXECUTABLE_NAME} gdi32 comctl32)
