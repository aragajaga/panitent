cmake_minimum_required(VERSION 2.8)
project(panitent)

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
    src/panitent.c
    src/document.c
    src/canvas.c
    src/primitives_context.c
    src/wu_primitives.c
    src/file_open.c
    src/toolshelf.c
    src/palette.c
    src/viewport.c
    src/settings.c
    src/debug.c
    src/new.c
    res/panitent.rc)
    
if(MSVC)
    set(SOURCE_EXE ${SOURCE_EXE} res/panitent.exe.manifest)
endif(MSVC)

add_executable(${PROJECT_NAME} ${SOURCE_EXE})
target_link_libraries(${PROJECT_NAME} shlwapi comctl32 msimg32 uxtheme)
