fcitx_add_addon_conf_file(fcitx-libpinyin.conf)
fcitx_add_configdesc_file(fcitx-libpinyin.desc)

fcitx_add_inputmethod_conf_file(pinyin-libpinyin.conf)
fcitx_add_inputmethod_conf_file(shuangpin-libpinyin.conf)
fcitx_add_inputmethod_conf_file(zhuyin-libpinyin.conf)

install(FILES bopomofo.png RENAME fcitx-bopomofo.png DESTINATION share/icons/hicolor/48x48/status/)

set(ZHUYIN_DATA_FILE_NAME model.text.20120921.tar.gz)
set(ZHUYIN_URL "https://github.com/downloads/fcitx/fcitx-libpinyin/${ZHUYIN_DATA_FILE_NAME}")
set(ZHUYIN_TAR "${CMAKE_CURRENT_SOURCE_DIR}/${ZHUYIN_DATA_FILE_NAME}")
set(ZHUYIN_ORGDATA gb_char.table gbk_char.table interpolation.text)
set(ZHUYIN_DATA bigram.db  gb_char.bin  gbk_char.bin  phrase_index.bin  pinyin_index.bin)

add_custom_target(zhuyin_data ALL DEPENDS ${ZHUYIN_DATA})

add_custom_command(OUTPUT ${ZHUYIN_ORGDATA}
                   COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/download.sh "${WGET}" "${ZHUYIN_URL}" "${ZHUYIN_TAR}"
                   COMMAND ${TAR} xzmvf ${ZHUYIN_TAR})
add_custom_command(OUTPUT ${ZHUYIN_DATA}
                   DEPENDS ${ZHUYIN_ORGDATA}
                   COMMAND ${LIBPINYIN_GEN_BINARY_FILES} --table-dir ${CMAKE_CURRENT_BINARY_DIR}
                   COMMAND ${LIBPINYIN_IMPORT_INTERPOLATION} < ${CMAKE_CURRENT_BINARY_DIR}/interpolation.text
                   COMMAND ${LIBPINYIN_GEN_UNIGRAM})

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/bigram.db
              ${CMAKE_CURRENT_BINARY_DIR}/gb_char.bin
              ${CMAKE_CURRENT_BINARY_DIR}/gbk_char.bin
              ${CMAKE_CURRENT_BINARY_DIR}/phrase_index.bin
              ${CMAKE_CURRENT_BINARY_DIR}/pinyin_index.bin
        DESTINATION ${FCITX_LIBPINYIN_ZHUYIN_DATADIR})
