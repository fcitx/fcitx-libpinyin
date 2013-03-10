install(FILES bopomofo.png RENAME fcitx-bopomofo.png
  DESTINATION share/icons/hicolor/48x48/status/)

set(_LIBPINYIN_TABLE art gb_char history nature scitech
                     culture gbk_char life society economy geology merged
                     sport)

foreach(f ${_LIBPINYIN_TABLE})
    set(_LIBPINYIN_TABLE_FILES ${_LIBPINYIN_TABLE_FILES} "${f}.table")
    set(_LIBPINYIN_BIN_FILES ${_LIBPINYIN_BIN_FILES} "${f}.bin")
    set(_LIBPINYIN_GEN_BIN_FILES ${_LIBPINYIN_GEN_BIN_FILES} "${CMAKE_CURRENT_BINARY_DIR}/${f}.bin")
endforeach()

set(ZHUYIN_DATA_FILE_NAME model.text.20130308.tar.gz)
set(ZHUYIN_URL "http://fcitx.googlecode.com/files/${ZHUYIN_DATA_FILE_NAME}")
set(ZHUYIN_ORGDATA ${_LIBPINYIN_TABLE_FILES} interpolation2.text)
set(ZHUYIN_DATA bigram.db ${_LIBPINYIN_BIN_FILES}  phrase_index.bin  pinyin_index.bin)

fcitx_download(zhuyin_data_tar "${ZHUYIN_URL}" "${ZHUYIN_DATA_FILE_NAME}"
  MD5SUM f908d19ebef9527fea06ea7d072500b0)
fcitx_extract(zhuyin-data-extract "${ZHUYIN_DATA_FILE_NAME}"
  DEPENDS zhuyin_data_tar OUTPUT ${ZHUYIN_ORGDATA})

add_custom_command(OUTPUT ${ZHUYIN_DATA}
  DEPENDS ${ZHUYIN_ORGDATA} zhuyin-data-extract
  COMMAND "${CMAKE_COMMAND}" -E remove ${ZHUYIN_DATA}
  COMMAND "${LIBPINYIN_GEN_BINARY_FILES}"
  --table-dir "${CMAKE_CURRENT_BINARY_DIR}"
  COMMAND "${LIBPINYIN_IMPORT_INTERPOLATION}"
  < "${CMAKE_CURRENT_BINARY_DIR}/interpolation2.text"
  COMMAND "${LIBPINYIN_GEN_UNIGRAM}")

add_custom_target(zhuyin_data ALL DEPENDS ${ZHUYIN_DATA})

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/bigram.db
              ${_LIBPINYIN_GEN_BIN_FILES}
              ${CMAKE_CURRENT_BINARY_DIR}/phrase_index.bin
              ${CMAKE_CURRENT_BINARY_DIR}/pinyin_index.bin
        DESTINATION ${FCITX_LIBPINYIN_ZHUYIN_DATADIR})
