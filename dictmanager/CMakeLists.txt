set(DICT_MANAGER_SRCS
    importer.cpp
    dictmanager.cpp
    main.cpp
    filelistmodel.cpp
    browserdialog.cpp
    filedownloader.cpp
    scelconverter.cpp
    erroroverlay.cpp
    )
fcitx_translate_add_sources(
    ${DICT_MANAGER_SRCS})

if (NOT ENABLE_QT)
    return()
endif ()

find_package(Qt5 5.7 REQUIRED COMPONENTS Core Gui WebEngineWidgets Network DBus)
find_package(FcitxQt5WidgetsAddons 1.1 REQUIRED)
find_package(FcitxQt5DBusAddons 1.1 REQUIRED)

include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ../src/
    )

include_directories (
    ${PROJECT_BINARY_DIR}
    ${LIBPINYIN_INCLUDE_DIR}
    ${FCITX4_FCITX_UTILS_INCLUDE_DIRS}
    ${FCITX4_FCITX_CONFIG_INCLUDE_DIRS}
    ${FCITX4_FCITX_INCLUDE_DIRS}
    ${LIBINTL_INCLUDE_DIR}
    ${GLIB2_INCLUDE_DIR}
    ${DBUS_INCLUDE_DIR}
    ${DBUS_ARCH_INCLUDE_DIR}
    )

link_directories(
${FCITX4_FCITX_UTILS_LIBRARY_DIRS}
${FCITX4_FCITX_CONFIG_LIBRARY_DIRS}
)

qt5_wrap_ui(DICT_MANAGER_SRCS dictmanager.ui browserdialog.ui)

add_library(fcitx-libpinyin-dictmanager MODULE ${DICT_MANAGER_SRCS})
set_target_properties(fcitx-libpinyin-dictmanager PROPERTIES
                      LINK_FLAGS "-Wl,--no-undefined" AUTOMOC TRUE)

target_link_libraries(fcitx-libpinyin-dictmanager
Qt5::Core Qt5::Gui Qt5::DBus Qt5::Network Qt5::WebEngineWidgets FcitxQt5::WidgetsAddons FcitxQt5::DBusAddons
${FCITX4_FCITX_UTILS_LIBRARIES}
${FCITX4_FCITX_CONFIG_LIBRARIES}
${LIBINTL_LIBRARIES}
)

install(TARGETS fcitx-libpinyin-dictmanager DESTINATION ${FCITX4_ADDON_INSTALL_DIR}/qt)
