#include "common.h"
#include "config.h"
#include <fcitx-utils/utils.h>
#include <fcitx-config/xdg.h>

char* FcitxLibPinyinGetSysPath(LIBPINYIN_LANGUAGE_TYPE type)
{
    char* syspath = NULL;
    if (type == LPLT_Simplified) {
        /* portable detect here */
        if (getenv("FCITXDIR")) {
            syspath = fcitx_utils_get_fcitx_path_with_filename("datadir", "libpinyin/data");
        } else {
            syspath = strdup(LIBPINYIN_PKGDATADIR "/data");
        }
    }
    else {
        /* portable detect here */
        if (getenv("FCITXDIR")) {
            syspath = fcitx_utils_get_fcitx_path_with_filename("pkgdatadir", "libpinyin/zhuyin_data");
        } else {
            syspath = strdup(FCITX_LIBPINYIN_ZHUYIN_DATADIR);
        }
    }
    return syspath;
}


char* FcitxLibPinyinGetUserPath(LIBPINYIN_LANGUAGE_TYPE type)
{
    char* user_path = NULL;
    if (type == LPLT_Simplified) {
        FILE* fp = FcitxXDGGetFileUserWithPrefix("libpinyin", "data/.place_holder", "w", NULL);
        if (fp)
            fclose(fp);
        FcitxXDGGetFileUserWithPrefix("libpinyin", "data", NULL, &user_path);
    }
    else {
        FILE* fp = FcitxXDGGetFileUserWithPrefix("libpinyin", "zhuyin_data/.place_holder", "w", NULL);
        if (fp)
            fclose(fp);
        FcitxXDGGetFileUserWithPrefix("libpinyin", "zhuyin_data", NULL, &user_path);
    }
    return user_path;
}
