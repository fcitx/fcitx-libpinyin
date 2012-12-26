install(FILES bopomofo.png RENAME fcitx-bopomofo.png
  DESTINATION share/icons/hicolor/48x48/status/)

set(ZHUYIN_DATA_FILE_NAME model.text.20120921.tar.gz)
set(ZHUYIN_URL "http://fcitx.googlecode.com/files/${ZHUYIN_DATA_FILE_NAME}")
set(ZHUYIN_TAR "${CMAKE_CURRENT_SOURCE_DIR}/${ZHUYIN_DATA_FILE_NAME}")
set(ZHUYIN_ORGDATA gb_char.table gbk_char.table interpolation.text)
set(ZHUYIN_DATA bigram.db  gb_char.bin  gbk_char.bin  phrase_index.bin  pinyin_index.bin)

fcitx_download(zhuyin_data_tar "${ZHUYIN_URL}" "${ZHUYIN_TAR}"
  MD5SUM 008cb5b10403d53a384d01de0345f0aa)

add_custom_command(OUTPUT ${ZHUYIN_ORGDATA}
  DEPENDS "${ZHUYIN_TAR}" zhuyin_data_tar
  COMMAND "${TAR}" xzmvf "${ZHUYIN_TAR}")

add_custom_command(OUTPUT ${ZHUYIN_DATA}
  DEPENDS ${ZHUYIN_ORGDATA}
  COMMAND ${LIBPINYIN_GEN_BINARY_FILES} --table-dir ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND ${LIBPINYIN_IMPORT_INTERPOLATION} < ${CMAKE_CURRENT_BINARY_DIR}/interpolation.text
  COMMAND ${LIBPINYIN_GEN_UNIGRAM})

add_custom_target(zhuyin_data ALL DEPENDS ${ZHUYIN_DATA})

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/bigram.db
              ${CMAKE_CURRENT_BINARY_DIR}/gb_char.bin
              ${CMAKE_CURRENT_BINARY_DIR}/gbk_char.bin
              ${CMAKE_CURRENT_BINARY_DIR}/phrase_index.bin
              ${CMAKE_CURRENT_BINARY_DIR}/pinyin_index.bin
        DESTINATION ${FCITX_LIBPINYIN_ZHUYIN_DATADIR})
