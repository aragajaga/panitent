cmake_minimum_required(VERSION 2.8)

project(panitent)
# find_package(PNG REQUIRED)
# include_directories(${PNG_INCLUDE_DIR})

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/")

if(MSVC)
    # MSVC
endif(MSVC)

if(MINGW)
    # Optimization flags
    set(CMAKE_C_FLAGS_RELEASE "-O2")
    set(CMAKE_C_FLAGS_DEBUG "-g -O0")
    set(CMAKE_C_FLAGS "-Wall")
    mark_as_advanced(CMAKE_C_FLAGS_COVERAGE)

    # Set windres as resource compiler
    set(CMAKE_RC_COMPILER_INIT windres)
    enable_language(RC)
    set(CMAKE_RC_COMPILE_OBJECT
        "<CMAKE_RC_COMPILER> <FLAGS> -O coff <DEFINES> -i <SOURCE> -o <OBJECT>")
endif(MINGW)

if(CMAKE_BUILD_TYPE STREQUAL "Coverage")
    message("Running coverage build")
    include(CodeCoverage)
    setup_target_for_coverage(${PROJECT_NAME}_coverage ctest ${PROJECT_NAME})
    
    set(CMAKE_C_FLAGS "-g -O0 -fprofile-arcs -ftest-coverage")
endif()    

enable_testing()
find_package(cmocka REQUIRED)
include_directories(${CMOCKA_INCLUDE_DIR})
add_executable(test_panitent tests/test_panitent.c)
add_test(test_panitent test_panitent)
target_link_libraries(test_panitent ${CMOCKA_LIBRARY})

set(SOURCE_EXE
    src/panitent.c
    src/file_open.c
    src/toolshelf.c
    src/viewport.c
    src/settings.c
    src/debug.c
    src/new.c
    res/panitent.rc)
    
if(MSVC)
set(SOURCE_EXE ${SOURCE_EXE} res/panitent.exe.manifest)
endif(MSVC)

add_executable(${PROJECT_NAME} ${SOURCE_EXE})
target_link_libraries(${PROJECT_NAME} shlwapi comctl32 msimg32)
# target_link_libraries(${PROJECT_NAME} shlwapi comctl32 msimg32 ${PNG_LIBRARY})
