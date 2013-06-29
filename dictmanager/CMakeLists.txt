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

if (ENABLE_QT)
find_package(Qt4 4.8 REQUIRED COMPONENTS QtCore QtGui QtWebkit QtNetwork)
pkg_check_modules(FCITX_QT "fcitx-qt>=4.2.8" REQUIRED )
endif (ENABLE_QT)

if (NOT QT_FOUND OR NOT ENABLE_QT)
    return()
endif (NOT QT_FOUND OR NOT ENABLE_QT)

include_directories(
    ${FCITX_QT_INCLUDE_DIRS}
    ${QT_QTCORE_INCLUDE_DIR}
    ${QT_QTGUI_INCLUDE_DIR}
    ${QT_QTWEBKIT_INCLUDE_DIR}
    ${QT_QTNETWORK_INCLUDE_DIR}
    ${QT_QTDBUS_INCLUDE_DIR}
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
${FCITX_QT_LIBRARY_DIRS}
)

qt4_wrap_ui(DICT_MANAGER_SRCS dictmanager.ui browserdialog.ui)

add_executable(fcitx-libpinyin-dictmanager ${DICT_MANAGER_SRCS})
set_target_properties(fcitx-libpinyin-dictmanager PROPERTIES AUTOMOC TRUE)

target_link_libraries(fcitx-libpinyin-dictmanager
${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${QT_QTDBUS_LIBRARY} ${QT_QTNETWORK_LIBRARY} ${QT_QTWEBKIT_LIBRARY}
${FCITX_QT_LIBRARIES}
${FCITX4_FCITX_UTILS_LIBRARIES}
${FCITX4_FCITX_CONFIG_LIBRARIES}
)

install(TARGETS fcitx-libpinyin-dictmanager DESTINATION bin)
