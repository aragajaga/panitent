cmake_minimum_required(VERSION 2.8)

project(panitent)

if(MSVC)
    # MSVC
endif(MSVC)

if(MINGW)
    # Optimization flags
    set(CMAKE_C_FLAGS_RELEASE "-O2")
    set(CMAKE_C_FLAGS_DEBUG "-O0 -g")
    set(CMAKE_C_FLAGS "-Wall")

    # Set windres as resource compiler
    set(CMAKE_RC_COMPILER_INIT windres)
    enable_language(RC)
    set(CMAKE_RC_COMPILE_OBJECT
        "<CMAKE_RC_COMPILER> <FLAGS> -O coff <DEFINES> -i <SOURCE> -o <OBJECT>")
endif(MINGW)

set(SOURCE_EXE
    src/panitent.c
    src/file_open.c
    src/toolshelf.c
    src/viewport.c
    src/settings.c
    res/panitent.rc)
    
if(MSVC)
set(SOURCE_EXE ${SOURCE_EXE} res/panitent.exe.manifest)
endif(MSVC)

add_executable(${PROJECT_NAME} WIN32 ${SOURCE_EXE})
target_link_libraries(${PROJECT_NAME} shlwapi comctl32 msimg32)
