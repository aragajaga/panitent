set(WIN32FW_SOURCES
    application.c
    dialog.c
    framework.c
    ../util/hashmap.c
    listbox.c
    propertygrid.c
    propertygridimpl.c
    tree_view.c
    util.c
    window.c
    windowmap.c
    )

add_library(win32fw STATIC ${WIN32FW_SOURCES})
target_compile_definitions(win32fw PUBLIC _UNICODE UNICODE)
target_compile_features(win32fw PUBLIC c_std_99)
if (MSVC)
  target_compile_options(win32fw PRIVATE /W3)
endif(MSVC)
