fcitx_add_addon_conf_file(fcitx-libpinyin.conf)
fcitx_add_configdesc_file(fcitx-libpinyin.desc)

fcitx_add_inputmethod_conf_file(pinyin-libpinyin.conf)
fcitx_add_inputmethod_conf_file(shuangpin-libpinyin.conf)
fcitx_add_inputmethod_conf_file(zhuyin-libpinyin.conf)

install(FILES bopomofo.png RENAME fcitx-bopomofo.png DESTINATION share/icons/hicolor/48x48/status/)