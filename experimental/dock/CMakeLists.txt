cmake_minimum_required(VERSION 3.17)
project(dock)

set(SOURCES
  src/main.c
  src/swatch.c
  src/dockhost.c
  res/dock.rc
)

set(EXECUTABLE_NAME ${PROJECT_NAME})

add_executable(${EXECUTABLE_NAME} WIN32 ${SOURCES})
target_precompile_headers(${EXECUTABLE_NAME} PUBLIC src/precomp.h)
target_link_libraries(${EXECUTABLE_NAME} gdi32 comctl32)
