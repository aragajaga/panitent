cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

project(panitent LANGUAGES C)

if(MINGW)
    # Optimization flags
    set(CMAKE_C_FLAGS_RELEASE "-O2")
    set(CMAKE_C_FLAGS_DEBUG "-g -O0")

    # Set windres as resource compiler
    set(CMAKE_RC_COMPILER_INIT windres)
    enable_language(RC)
    set(CMAKE_RC_COMPILE_OBJECT
        "<CMAKE_RC_COMPILER> <FLAGS> -O coff <DEFINES> -i <SOURCE> -o <OBJECT>")
endif()

set(SOURCE_EXE
    src/bresenham.c
    src/canvas.c
    src/color_context.c
    src/debug.c
    src/dockhost.c
    src/document.c
    src/file_open.c
    src/new.c
    src/option_bar.c
    src/palette.c
    src/panitent.c
    src/primitives_context.c
    src/settings.c
    src/swatch.c
    src/swatch2.c
    src/toolbox.c
    src/viewport.c
    src/wu_primitives.c
    src/crefptr.c
    src/wic.c
    res/panitent.rc)

if(MSVC)
    set(SOURCE_EXE ${SOURCE_EXE} res/panitent.exe.manifest)
endif(MSVC)

add_executable(${PROJECT_NAME} WIN32 ${SOURCE_EXE})

if (MINGW)
  target_compile_definitions(${PROJECT_NAME} PUBLIC _UNICODE UNICODE)
  target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic
      -Werror -Wno-unused)
  target_link_options(${PROJECT_NAME} PRIVATE -municode)
endif(MINGW)

target_link_libraries(${PROJECT_NAME} shlwapi comctl32 msimg32 uxtheme
    windowscodecs)

