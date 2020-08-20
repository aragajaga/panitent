cmake_minimum_required(VERSION 2.8)
project(panitent C)

if(MINGW)
    # Optimization flags
    set(CMAKE_C_FLAGS_RELEASE "-O2")
    set(CMAKE_C_FLAGS_DEBUG "-g -O0")
    set(CMAKE_C_FLAGS "-Wall")

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
    src/document.c
    src/file_open.c
    src/new.c
    src/option_bar.c
    src/palette.c
    src/panitent.c
    src/primitives_context.c
    src/settings.c
    src/swatch.c
    src/toolshelf.c
    src/viewport.c
    src/wu_primitives.c
    res/panitent.rc)
    
if(MSVC)
    set(SOURCE_EXE ${SOURCE_EXE} res/panitent.exe.manifest)
endif(MSVC)

add_executable(${PROJECT_NAME} ${SOURCE_EXE})
target_link_libraries(${PROJECT_NAME} shlwapi comctl32 msimg32 uxtheme)
