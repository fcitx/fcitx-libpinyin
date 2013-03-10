include_directories(../src/
    ${LIBPINYIN_INCLUDE_DIR}
    ${GLIB2_INCLUDE_DIR}
    ${PROJECT_BINARY_DIR}
)

add_executable(sptest sp.cpp )

target_link_libraries(sptest
  ${LIBPINYIN_LIBRARIES}
  ${LIBINTL_LIBRARIES}
  ${GLIB2_LIBRARIES}
  )

add_test(sptest sptest)
