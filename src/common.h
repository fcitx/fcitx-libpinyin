#ifndef FCITX_LIBPINYIN_COMMON_H
#define FCITX_LIBPINYIN_COMMON_H


enum LIBPINYIN_TYPE {
    LPT_Pinyin,
    LPT_Zhuyin,
    LPT_Shuangpin
};

enum LIBPINYIN_LANGUAGE_TYPE {
    LPLT_Simplified,
    LPLT_Traditional
};

#define FCITX_LIBPINYIN_PATH "/libpinyin"
#define FCITX_LIBPINYIN_INTERFACE "org.fcitx.Fcitx.LibPinyin"

#endif // FCITX_LIBPINYIN_COMMON_H
