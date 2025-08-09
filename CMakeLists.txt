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

add_subdirectory(src/win32)

set(C_SOURCES
    src/binview/audioplayer.c
    src/binview/audiosource.c
    src/binview/binview.c
    src/binview/binviewdlg.c
    src/experimental/bubble.c
    src/experimental/captionglyphs.c
    src/experimental/dockhostcomposite.c
    src/experimental/dockpanel.c
    src/experimental/dockwindow.c
    src/grimstroke/bresenhamshapestrategy.c
    src/grimstroke/msaa.c
    src/grimstroke/msaaplotterproxy.c
    src/grimstroke/pixelbuffer.c
    src/grimstroke/plotter.c
    src/grimstroke/polygon.c
    src/grimstroke/shapecontext.c
    src/grimstroke/shapestrategy.c
    src/grimstroke/wushapestrategy.c
    src/s11n/bsonserializer.c
    src/s11n/jsonserializer.c
    src/s11n/serializable_example.c
    src/s11n/xmlserializer.c
    src/sharing/activitysharingclient.c
    src/sharing/activitysharingmanager.c
    src/sharing/activitystubdialog.c
    src/sharing/discordasp.c
    src/shell/args.c
    src/shell/pathutil.c
    src/tools/brushtool.c
    src/tools/circletool.c
    src/tools/erasertool.c
    src/tools/filltool.c
    src/tools/linetool.c
    src/tools/penciltool.c
    src/tools/pickertool.c
    src/tools/pointertool.c
    src/tools/rectangletool.c
    src/tools/texttool.c
    src/util/bytestream.c
    src/util/hashmap.c
    src/util/list.c
    src/util/pntrtti.c
    src/util/pntstring.c
    src/util/queue.c
    src/util/rbhashmap.c
    src/util/stack.c
    src/util/tree.c
    src/util/utf.c
    src/util/vector.c
    src/aboutbox.c
    src/appcmd.c
    src/brush.c
    src/brush_context.c
    src/canvas.c
    src/checker.c
    src/color_context.c
    src/controllibrary.c
    src/crashhandler.c
    src/crefptr.c
    src/debug.c
    src/dockhost.c
    src/dockinspectordialog.c
    src/document.c
    src/file_open.c
    src/flexible.c
    src/floatingwindowcontainer.c
    src/glwindow.c
    src/history.c
    src/layeredwindow.c
    src/layerswindow.c
    src/log.c
    src/log_window.c
    src/msthemeloader.c
    src/new.c
    src/option_bar.c
    src/palette.c
    src/palette_settings_dlg.c
    src/palette_window.c
    src/panitent.c
    src/panitentapp.c
    src/panitentwindow.c
    src/pentablet.c
    src/playground.c
    src/pntini.c
    src/pntxml.c
    src/propgriddialog.c
    src/rbhashmapviz.c
    src/settings.c
    src/settings_dialog.c
    src/settings_wnd.c
    src/splittercontainer.c
    src/swatch.c
    src/swatch2.c
    src/toolbox.c
    src/toolwndframe.c
    src/util.c
    src/verifycheck.c
    src/viewport.c
    src/wic.c
    src/windowstub.c
    src/workspacecontainer.c
    )

set(SOURCE_EXE ${C_SOURCES} res/panitent.rc)

if(MSVC)
  set(SOURCE_EXE ${SOURCE_EXE} res/panitent.exe.manifest)

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
  target_compile_options(${PROJECT_NAME} PRIVATE /W3)
  # target_link_options(${PROJECT_NAME} PRIVATE /WX)
endif(MSVC)

target_link_libraries(${PROJECT_NAME} win32fw shlwapi comctl32 uxtheme msimg32 windowscodecs dbghelp opengl32 dwmapi)
