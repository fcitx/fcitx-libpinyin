#include "common.h"
#include "config.h"
#include <fcitx-utils/utils.h>
#include <fcitx-config/xdg.h>

char* FcitxLibPinyinGetSysPath(LIBPINYIN_LANGUAGE_TYPE type)
{
    char* syspath = NULL;
#ifdef LIBPINYIN_TOOLS_FOUND
    if (type == LPLT_Simplified) {
#endif
        /* portable detect here */
        if (getenv("FCITXDIR")) {
            syspath = fcitx_utils_get_fcitx_path_with_filename("datadir", "libpinyin/data");
        } else {
            syspath = strdup(LIBPINYIN_PKGDATADIR "/data");
        }
#ifdef LIBPINYIN_TOOLS_FOUND
    }
    else {
        /* portable detect here */
        if (getenv("FCITXDIR")) {
            syspath = fcitx_utils_get_fcitx_path_with_filename("pkgdatadir", "libpinyin/zhuyin_data");
        } else {
            syspath = strdup(FCITX_LIBPINYIN_ZHUYIN_DATADIR);
        }
    }
#endif
    return syspath;
}


char* FcitxLibPinyinGetUserPath(LIBPINYIN_LANGUAGE_TYPE type)
{
    char* user_path = NULL;
#ifdef LIBPINYIN_TOOLS_FOUND
    if (type == LPLT_Simplified) {
#endif
        FILE* fp = FcitxXDGGetFileUserWithPrefix("libpinyin", "data/.place_holder", "w", NULL);
        if (fp)
            fclose(fp);
        FcitxXDGGetFileUserWithPrefix("libpinyin", "data", NULL, &user_path);
#ifdef LIBPINYIN_TOOLS_FOUND
    }
    else {
        FILE* fp = FcitxXDGGetFileUserWithPrefix("libpinyin", "zhuyin_data/.place_holder", "w", NULL);
        if (fp)
            fclose(fp);
        FcitxXDGGetFileUserWithPrefix("libpinyin", "zhuyin_data", NULL, &user_path);
    }
#endif
    return user_path;
}
