cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

project(panitent C)

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

set(C_SOURCES
    src/bresenham.c
    src/brush.c
    src/canvas.c
    src/color_context.c
    src/debug.c
    src/dockhost.c
    src/dock_manager.c
    src/dock_site.c
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
    src/history.c
    src/checker.c
    src/pentablet.c
    src/flexible.c
    src/settings_dialog.c
    src/util.c
    src/util/list.c
    src/dock_guides.c
    src/log.c
    src/log_window.c
    )

set(SOURCE_EXE ${C_SOURCES} res/panitent.rc)

if(MSVC)
  set(SOURCE_EXE ${SOURCE_EXE} res/panitent.exe.manifest)

  set(HEADERS
      src/bresenham.h
      src/brush.h
      src/brush_context.h
      src/canvas.h
      src/checker.h
      src/color_context.h
      src/crefptr.h
      src/debug.h
      src/discordsdk.h
      src/dockhost.h
      src/dock_guides.h
      src/dock_site.h
      src/document.h
      src/file_open.h
      src/flexible.h
      src/history.h
      src/new.h
      src/option_bar.h
      src/palette.h
      src/pencil_tool.h
      src/pentablet.h
      src/precomp.h
      src/primitives_context.h
      src/resource.h
      src/settings.h
      src/settings_dialog.h
      src/swatch.h
      src/swatch2.h
      src/test.h
      src/tool.h
      src/toolbox.h
      src/util.h
      src/viewport.h
      src/wic.h
      src/wu_primitives.h
      )

  source_group("Headers" FILES ${HEADERS})

  set(SOURCE_EXE ${SOURCE_EXE} ${HEADERS})
endif(MSVC)

set_source_files_properties(${C_SOURCES} PROPERTIES LANGUAGE C)

add_executable(${PROJECT_NAME} WIN32 ${SOURCE_EXE})

option(HAS_DISCORDRPC "Use Discord RPC" OFF)
if (HAS_DISCORDRPC)
  target_sources(${PROJECT_NAME} PUBLIC src/discordsdk.c)
  target_include_directories(${PROJECT_NAME} PUBLIC
      "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/include")
  target_link_directories(${PROJECT_NAME} PUBLIC
      "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/lib")
  target_compile_definitions(${PROJECT_NAME} PUBLIC HAS_DISCORDSDK)

  target_link_libraries(${PROJECT_NAME} "discord_game_sdk.dll.lib")
endif()

target_compile_definitions(${PROJECT_NAME} PUBLIC _UNICODE UNICODE)
target_compile_features(${PROJECT_NAME} PUBLIC c_std_99)

if (MINGW)
  target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic
      -Werror -Wno-unused)
  target_link_options(${PROJECT_NAME} PRIVATE -municode)
endif(MINGW)

if (MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX)
  target_link_options(${PROJECT_NAME} PRIVATE /WX)
endif(MSVC)

target_link_libraries(${PROJECT_NAME} shlwapi comctl32 msimg32 uxtheme
    windowscodecs dbghelp)

