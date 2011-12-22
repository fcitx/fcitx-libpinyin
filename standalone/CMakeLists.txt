include_directories (
    ${PROJECT_BINARY_DIR}
    ${LIBPINYIN_INCLUDE_DIRS}
    ${FCITX4_FCITX_UTILS_INCLUDE_DIRS}
    ${FCITX4_FCITX_CONFIG_INCLUDE_DIRS}
    ${FCITX4_FCITX_INCLUDE_DIRS}
    ${LIBINTL_INCLUDE_DIR}
    )
    
add_executable(standalone_test main.cpp )
target_link_libraries (standalone_test
                       ${LIBPINYIN_LIBRARIES}
                       ${LIBINTL_LIBRARIES}
                       )
