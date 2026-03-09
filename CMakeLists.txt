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
    src/alphamask.c
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
    src/documentsessionmodel.c
    src/documentsessionpersist.c
    src/dockdefaultlayoutmodel.c
    src/docklayout.c
    src/dockgroup.c
    src/dockhost.c
    src/dockhostcaption.c
    src/dockhostdrag.c
    src/dockhostinput.c
    src/dockhostlayout.c
    src/dockhostmetrics.c
    src/dockhostautohide.c
    src/dockhostmodelapply.c
    src/dockhostmutate.c
    src/dockhostpanelmenu.c
    src/dockhostpaint.c
    src/dockhostrestore.c
    src/dockhostruntime.c
    src/dockhosttree.c
    src/dockhostzone.c
    src/dockfloatingmodel.c
    src/dockfloatingpersist.c
    src/floatingdocumentlayoutmodel.c
    src/floatingdocumentlayoutpersist.c
    src/floatingdocumentsessionmodel.c
    src/floatingdocumentsessionpersist.c
    src/docklayoutpersist.c
    src/dockpolicy.c
    src/dockmodel.c
    src/dockmodelmatch.c
    src/dockmodelops.c
    src/dockmodelbuild.c
    src/dockmodelio.c
    src/dockmodelvalidate.c
    src/dockshell.c
    src/docktypes.c
    src/dockviewcatalog.c
    src/dockviewfactory.c
    src/dockinspectordialog.c
    src/document.c
    src/documentrecovery.c
    src/file_open.c
    src/flexible.c
    src/floatingchildhost.c
    src/floatingdocumenthost.c
    src/floatingdockpolicy.c
    src/floatingwindowcontainer.c
    src/glwindow.c
    src/history.c
    src/layeredwindow.c
    src/layerswindow.c
    src/log.c
    src/log_window.c
    src/msthemeloader.c
    src/new.c
    src/oledroptarget.c
    src/option_bar.c
    src/palette.c
    src/palette_settings_dlg.c
    src/palette_window.c
    src/panitent.c
    src/panitentapp.c
    src/panitentwindow.c
    src/pentablet.c
    src/persistfile.c
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
    src/recoverystore.c
    src/recoverystorepersist.c
    src/recoverystoregc.c
    src/toolbox.c
    src/theme.c
    src/toolwndframe.c
    src/util.c
    src/verifycheck.c
    src/viewport.c
    src/wic.c
    src/windowstub.c
    src/workspacedockpolicy.c
    src/workspacecontainer.c
    src/windowlayoutcatalog.c
    src/windowlayoutprofile.c
    src/windowlayoutmanager.c
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

option(PANITENT_BUILD_TESTS "Build unit tests for Panitent" ON)
if(PANITENT_BUILD_TESTS)
  enable_testing()

  add_executable(panitent_dock_tests
    tests/docklayout_tests.c
    src/documentsessionmodel.c
    src/dockdefaultlayoutmodel.c
    src/floatingdocumentlayoutmodel.c
    src/floatingdocumentsessionmodel.c
    src/dockfloatingmodel.c
    src/dockmodel.c
    src/dockmodelmatch.c
    src/dockmodelops.c
    src/dockmodelbuild.c
    src/dockmodelio.c
    src/dockmodelvalidate.c
    src/floatingdockpolicy.c
    src/persistfile.c
    src/dockgroup.c
    src/dockshell.c
    src/docklayout.c
    src/dockpolicy.c
    src/docktypes.c
    src/dockviewcatalog.c
    src/recoverystore.c
    src/windowlayoutcatalog.c
    src/windowlayoutprofile.c
    src/shell/pathutil.c
    src/workspacedockpolicy.c
  )

  target_compile_features(panitent_dock_tests PRIVATE c_std_99)
  target_include_directories(panitent_dock_tests PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
  )
  target_compile_definitions(panitent_dock_tests PRIVATE _UNICODE UNICODE)

  if(MSVC)
    target_compile_options(panitent_dock_tests PRIVATE /W3)
  endif()

  add_test(NAME panitent_dock_layout COMMAND panitent_dock_tests)

  add_executable(panitent_runtime_tests
    tests/dockruntime_tests.c
    ${C_SOURCES}
  )

  target_compile_features(panitent_runtime_tests PRIVATE c_std_99)
  target_include_directories(panitent_runtime_tests PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
  )
  target_compile_definitions(panitent_runtime_tests PRIVATE _UNICODE UNICODE)

  if(MSVC)
    target_compile_options(panitent_runtime_tests PRIVATE /W3)
  endif()

  target_link_libraries(
    panitent_runtime_tests
    win32fw
    shlwapi
    comctl32
    uxtheme
    msimg32
    windowscodecs
    dbghelp
    opengl32
    dwmapi
  )

  add_test(NAME panitent_runtime_layout COMMAND panitent_runtime_tests)
endif()
