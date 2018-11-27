cmake_minimum_required(VERSION 2.8)

project(panitent)

if(MINGW)
    set(CMAKE_RC_COMPILER_INIT windres)
    ENABLE_LANGUAGE(RC)
    SET(CMAKE_RC_COMPILE_OBJECT
        "<CMAKE_RC_COMPILER> <FLAGS> -O coff <DEFINES> -i <SOURCE> -o <OBJECT>")
endif(MINGW)

set(SOURCE_EXE
    src/panitent.c
    src/file_open.c
    src/toolshelf.c
    src/viewport.c
    src/panitent.rc)

include_directories("${CMAKE_SOURCE_DIR}/mingw/include")
include_directories("${CMAKE_SOURCE_DIR}/src")
link_directories("${CMAKE_SOURCE_DIR}/mingw/lib")

add_executable(${PROJECT_NAME} ${SOURCE_EXE})
target_link_libraries(${PROJECT_NAME} propsys shlwapi)
