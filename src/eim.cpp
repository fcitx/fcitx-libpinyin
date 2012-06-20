/***************************************************************************
 *   Copyright (C) 2010~2011 by CSSlayer                                   *
 *   wengxt@gmail.com                                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.              *
 ***************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <pinyin.h>
#include <fcitx/ime.h>
#include <fcitx-config/hotkey.h>
#include <fcitx-config/xdg.h>
#include <fcitx-utils/log.h>
#include <fcitx-config/fcitx-config.h>
#include <fcitx-utils/utils.h>
#include <fcitx/instance.h>
#include <fcitx/keys.h>
#include <fcitx/module.h>
#include <fcitx/context.h>
#include <fcitx/module/punc/punc.h>
#include <string>
#include <libintl.h>

#include "config.h"
#include "eim.h"
#include "enummap.h"
#include "pystring.h"


#define FCITX_LIBPINYIN_MAX(x, y) ((x) > (y)? (x) : (y))
#define FCITX_LIBPINYIN_MIN(x, y) ((x) < (y)? (x) : (y))

#ifdef __cplusplus
extern "C" {
#endif
    FCITX_EXPORT_API
    FcitxIMClass ime = {
        FcitxLibpinyinCreate,
        FcitxLibpinyinDestroy
    };

    FCITX_EXPORT_API
    int ABI_VERSION = FCITX_ABI_VERSION;
#ifdef __cplusplus
}
#endif

typedef struct _FcitxLibpinyinCandWord {
    boolean issentence;
    boolean ispunc;
    lookup_candidate_t token;
} FcitxLibpinyinCandWord;

typedef struct _FcitxLibpinyinFixed {
    int len;
    lookup_candidate_t token;
} FcitxLibpinyinFixed;

CONFIG_DESC_DEFINE(GetLibpinyinConfigDesc, "fcitx-libpinyin.desc")

boolean LoadLibpinyinConfig(FcitxLibpinyinConfig* fs);
static void SaveLibpinyinConfig(FcitxLibpinyinConfig* fs);
static void ConfigLibpinyin(FcitxLibpinyinAddonInstance* libpinyin);
static int LibpinyinGetOffset(FcitxLibpinyin* libpinyin);
static void FcitxLibpinyinSave(void *arg);
static bool LibpinyinCheckZhuyinKey(FcitxKeySym sym, FCITX_ZHUYIN_LAYOUT layout, boolean useTone) ;
inline char* LibPinyinTransString(FcitxLibpinyin* libpinyin, char* sentence);
char* LibpinyinGetSentence(FcitxLibpinyin* libpinyin);
char* LibpinyinTransToken(FcitxLibpinyin* libpinyin, lookup_candidate_t* token);

static const char *input_keys [] = {
     "1qaz2wsxedcrfv5tgbyhnujm8ik,9ol.0p;/-",       /* standard kb */
     "1234567890-qwertyuiopasdfghjkl;zxcvbn",       /* IBM */
     "2wsx3edcrfvtgb6yhnujm8ik,9ol.0p;/-['=",       /* Gin-yieh */
     "bpmfdtnlvkhg7c,./j;'sexuaorwiqzy890-=",       /* ET  */
     0
};

static const char *tone_keys [] = {
     "7634 ",       /* standard kb */
     "/m,. ",       /* IBM */
     "1qaz ",       /* Gin-yieh */
     "1234 ",       /* ET  */
     0
};

static const FcitxKeyState candidateModifierMap[] = {
    FcitxKeyState_Ctrl,
    FcitxKeyState_Alt,
    FcitxKeyState_Shift,
};

static const FcitxHotkey FCITX_LIBPINYIN_SHIFT_ENTER[2] = {
    {NULL, FcitxKey_Return, FcitxKeyState_Shift},
    {NULL, FcitxKey_None, FcitxKeyState_None}
};

bool LibpinyinCheckZhuyinKey(FcitxKeySym sym, FCITX_ZHUYIN_LAYOUT layout, boolean useTone) {
    char key = sym & 0xff;
    const char* keys = input_keys[layout];
    const char* tones = tone_keys[layout];
    if (strchr(keys, key))
        return true;

    if (useTone && strchr(tones, key))
        return true;

    return false;
}

int LibpinyinGetOffset(FcitxLibpinyin* libpinyin)
{
    GArray* array = libpinyin->fixed_string;
    int sum = 0;
    for (int i = 0; i < array->len; i ++)
    {
        FcitxLibpinyinFixed* f = &g_array_index(array, FcitxLibpinyinFixed, i);
        sum += f->len;
    }
    return sum;
}

int LibpinyinGetPinyinOffset(FcitxLibpinyin* libpinyin)
{
    int offset = LibpinyinGetOffset(libpinyin);
    int pyoffset = 0;
    int i = FCITX_LIBPINYIN_MIN(offset, libpinyin->inst->m_pinyin_key_rests->len) - 1;
    if (i >= 0)
    {
        PinyinKeyPos* pykey = &g_array_index(libpinyin->inst->m_pinyin_key_rests, PinyinKeyPos, i);
        pyoffset = pykey->m_raw_end;
    }
    return pyoffset;
}

/**
 * @brief Reset the status.
 *
 **/
__EXPORT_API
void FcitxLibpinyinReset (void* arg)
{
    FcitxLibpinyin* libpinyin = (FcitxLibpinyin*) arg;
    libpinyin->buf[0] = '\0';
    libpinyin->cursor_pos = 0;
    if (libpinyin->fixed_string->len > 0)
        g_array_remove_range(libpinyin->fixed_string, 0, libpinyin->fixed_string->len);
    if (libpinyin->inst)
        pinyin_reset(libpinyin->inst);
}

size_t FcitxLibpinyinParse(FcitxLibpinyin* libpinyin, const char* str)
{
    switch(libpinyin->type)
    {
        case LPT_Pinyin:
            return pinyin_parse_more_full_pinyins(libpinyin->inst, str);
        case LPT_Shuangpin:
            return pinyin_parse_more_double_pinyins(libpinyin->inst, str);
        case LPT_Zhuyin:
            return pinyin_parse_more_chewings(libpinyin->inst, str);
    }
    return 0;
}

/**
 * @brief Process Key Input and return the status
 *
 * @param keycode keycode from XKeyEvent
 * @param state state from XKeyEvent
 * @param count count from XKeyEvent
 * @return INPUT_RETURN_VALUE
 **/
__EXPORT_API
INPUT_RETURN_VALUE FcitxLibpinyinDoInput(void* arg, FcitxKeySym sym, unsigned int state)
{
    FcitxLibpinyin* libpinyin = (FcitxLibpinyin*) arg;
    FcitxLibpinyinConfig* config = &libpinyin->owner->config;
    FcitxInputState* input = FcitxInstanceGetInputState(libpinyin->owner->owner);

    if (FcitxHotkeyIsHotKeySimple(sym, state))
    {
        /* there is some special case that ';' is used */
        if (FcitxHotkeyIsHotKeyLAZ(sym, state)
            || sym == '\''
            || (FcitxHotkeyIsHotKey(sym, state, FCITX_SEMICOLON) && libpinyin->type == LPT_Shuangpin && (config->spScheme == FCITX_SHUANG_PIN_MS || config->spScheme == FCITX_SHUANG_PIN_ZIGUANG))
            || (libpinyin->type == LPT_Zhuyin && LibpinyinCheckZhuyinKey(sym, config->zhuyinLayout, config->useTone))
        )
        {
            if (strlen(libpinyin->buf) == 0 && (sym == '\'' || sym == ';'))
                return IRV_TO_PROCESS;

            if (strlen(libpinyin->buf) < MAX_PINYIN_INPUT)
            {
                size_t len = strlen(libpinyin->buf);
                if (libpinyin->buf[libpinyin->cursor_pos] != 0)
                {
                    memmove(libpinyin->buf + libpinyin->cursor_pos + 1, libpinyin->buf + libpinyin->cursor_pos, len - libpinyin->cursor_pos);
                }
                libpinyin->buf[len + 1] = 0;
                libpinyin->buf[libpinyin->cursor_pos] = (char) (sym & 0xff);
                libpinyin->cursor_pos ++;

                size_t parselen = FcitxLibpinyinParse(libpinyin, libpinyin->buf);

                if (parselen == 0 && strlen(libpinyin->buf) == 1 && libpinyin->type != LPT_Shuangpin
                    && !(libpinyin->type == LPT_Pinyin && !libpinyin->owner->config.incomplete)
                    && !(libpinyin->type == LPT_Zhuyin && !libpinyin->owner->config.chewingIncomplete))
                {
                    FcitxLibpinyinReset(libpinyin);
                    return IRV_TO_PROCESS;
                }
                return IRV_DISPLAY_CANDWORDS;
            }
            else
                return IRV_DO_NOTHING;
        }
    }

    if (FcitxHotkeyIsHotKey(sym, state, FCITX_SPACE)
        || (libpinyin->type == LPT_Zhuyin && FcitxHotkeyIsHotKey(sym, state, FCITX_ENTER)))
    {
        size_t len = strlen(libpinyin->buf);
        if (len == 0)
            return IRV_TO_PROCESS;

        return FcitxCandidateWordChooseByIndex(FcitxInputStateGetCandidateList(input), 0);
    }

    if (FcitxHotkeyIsHotKey(sym, state, FCITX_LIBPINYIN_SHIFT_ENTER)) {
        size_t len = strlen(libpinyin->buf);
        if (len == 0)
            return IRV_TO_PROCESS;

        strcpy(FcitxInputStateGetOutputString(input), libpinyin->buf);

        return IRV_COMMIT_STRING;
    }

    if (FcitxHotkeyIsHotKey(sym, state, FCITX_BACKSPACE) || FcitxHotkeyIsHotKey(sym, state, FCITX_DELETE))
    {
        if (strlen(libpinyin->buf) > 0)
        {
            int offset = LibpinyinGetOffset(libpinyin);
            if (offset != 0 && FcitxHotkeyIsHotKey(sym, state, FCITX_BACKSPACE))
            {
                g_array_remove_index_fast(libpinyin->fixed_string, libpinyin->fixed_string->len - 1);
                pinyin_clear_constraint(libpinyin->inst, LibpinyinGetOffset(libpinyin));
            }
            else
            {
                if (FcitxHotkeyIsHotKey(sym, state, FCITX_BACKSPACE))
                {
                    if (libpinyin->cursor_pos > 0)
                        libpinyin->cursor_pos -- ;
                    else
                        return IRV_DO_NOTHING;
                }
                size_t len = strlen(libpinyin->buf);
                if (libpinyin->cursor_pos == (int)len)
                    return IRV_DO_NOTHING;
                memmove(libpinyin->buf + libpinyin->cursor_pos, libpinyin->buf + libpinyin->cursor_pos + 1, len - libpinyin->cursor_pos - 1);
                libpinyin->buf[strlen(libpinyin->buf) - 1] = 0;
                if (libpinyin->buf[0] == '\0')
                    return IRV_CLEAN;
                else
                    FcitxLibpinyinParse(libpinyin, libpinyin->buf);
            }
            return IRV_DISPLAY_CANDWORDS;
        }
        else
            return IRV_TO_PROCESS;
    }
    else
    {
        if (strlen(libpinyin->buf) > 0)
        {
            if (FcitxHotkeyIsHotKey(sym, state, FCITX_LEFT))
            {
                if (libpinyin->cursor_pos > 0)
                {
                    if ( libpinyin->cursor_pos == LibpinyinGetPinyinOffset(libpinyin))
                    {
                        g_array_remove_index_fast(libpinyin->fixed_string, libpinyin->fixed_string->len - 1);
                        pinyin_clear_constraint(libpinyin->inst, LibpinyinGetOffset(libpinyin));
                        return IRV_DISPLAY_CANDWORDS;
                    }
                    else
                    {
                        libpinyin->cursor_pos--;
                        return IRV_DISPLAY_CANDWORDS;
                    }
                }

                return IRV_DO_NOTHING;
            }
            else if (FcitxHotkeyIsHotKey(sym, state, FCITX_RIGHT))
            {
                size_t len = strlen(libpinyin->buf);
                if (libpinyin->cursor_pos < (int) len)
                {
                    libpinyin->cursor_pos ++ ;
                    return IRV_DISPLAY_CANDWORDS;
                }
                return IRV_DO_NOTHING;
            }
            else if (FcitxHotkeyIsHotKey(sym, state, FCITX_HOME))
            {
                int offset = LibpinyinGetPinyinOffset(libpinyin);
                if ( libpinyin->cursor_pos != offset)
                {
                    libpinyin->cursor_pos = offset;
                    return IRV_DISPLAY_CANDWORDS;
                }
                return IRV_DO_NOTHING;
            }
            else if (FcitxHotkeyIsHotKey(sym, state, FCITX_END))
            {
                size_t len = strlen(libpinyin->buf);
                if (libpinyin->cursor_pos != (int) len)
                {
                    libpinyin->cursor_pos = len ;
                    return IRV_DISPLAY_CANDWORDS;
                }
                return IRV_DO_NOTHING;
            }
        }
        else {
            return IRV_TO_PROCESS;
        }
    }
    return IRV_TO_PROCESS;
}

char* FcitxLibpinyinGetSysPath(LIBPINYIN_LANGUAGE_TYPE type)
{
    char* syspath = NULL;
    if (type == LPLT_Simplified) {
#if FCITX_CHECK_VERSION(4,2,1)
        /* portable detect here */
        if (getenv("FCITXDIR")) {
            syspath = fcitx_utils_get_fcitx_path_with_filename("datadir", "libpinyin/data");
        }
        else
#endif
        {
            syspath = strdup(LIBPINYIN_PKGDATADIR "/data");
        }
    }
    else {
#if FCITX_CHECK_VERSION(4,2,1)
        /* portable detect here */
        if (getenv("FCITXDIR")) {
            syspath = fcitx_utils_get_fcitx_path_with_filename("pkgdatadir", "libpinyin/zhuyin_data");
        }
        else
#endif
        {
            syspath = strdup(FCITX_LIBPINYIN_ZHUYIN_DATADIR);
        }
    }
    return syspath;
}


char* FcitxLibpinyinGetUserPath(LIBPINYIN_LANGUAGE_TYPE type)
{
    char* user_path = NULL;
    if (type == LPLT_Simplified) {
        FILE* fp = FcitxXDGGetFileUserWithPrefix("libpinyin", "data/.place_holder", "w", NULL);
        if (fp)
            fclose(fp);
        FcitxXDGGetFileUserWithPrefix("libpinyin", "data", NULL, &user_path);
        FcitxLog(INFO, "Libpinyin storage path %s", user_path);
    }
    else {
        FILE* fp = FcitxXDGGetFileUserWithPrefix("libpinyin", "zhuyin_data/.place_holder", "w", NULL);
        if (fp)
            fclose(fp);
        FcitxXDGGetFileUserWithPrefix("libpinyin", "zhuyin_data", NULL, &user_path);
    }
    return user_path;
}

void FcitxLibpinyinLoad(FcitxLibpinyin* libpinyin)
{
    if (libpinyin->inst != NULL)
        return;

    FcitxLibpinyinAddonInstance* libpinyinaddon = libpinyin->owner;

    if (libpinyin->type == LPT_Zhuyin && libpinyin->owner->zhuyin_context == NULL) {
        char* user_path = FcitxLibpinyinGetUserPath(libpinyinaddon->config.bSimplifiedDataForZhuyin ? LPLT_Simplified : LPLT_Traditional );
        char* syspath = FcitxLibpinyinGetSysPath(libpinyinaddon->config.bSimplifiedDataForZhuyin ? LPLT_Simplified : LPLT_Traditional );
        libpinyinaddon->zhuyin_context = pinyin_init( syspath, user_path);
        free(user_path);
        free(syspath);
    }

    if (libpinyin->type != LPT_Zhuyin && libpinyin->owner->pinyin_context == NULL) {
        char* user_path = FcitxLibpinyinGetUserPath(libpinyinaddon->config.bTraditionalDataForPinyin ? LPLT_Traditional : LPLT_Simplified );
        char* syspath = FcitxLibpinyinGetSysPath(libpinyinaddon->config.bTraditionalDataForPinyin ? LPLT_Traditional : LPLT_Simplified );
        libpinyinaddon->pinyin_context = pinyin_init(syspath, user_path);
        free(user_path);
        free(syspath);
    }

    if (libpinyin->type == LPT_Zhuyin)
        libpinyin->inst = pinyin_alloc_instance(libpinyinaddon->zhuyin_context);
    else
        libpinyin->inst = pinyin_alloc_instance(libpinyinaddon->pinyin_context);

    ConfigLibpinyin(libpinyinaddon);
}

boolean FcitxLibpinyinInit(void* arg)
{
    FcitxLibpinyin* libpinyin = (FcitxLibpinyin*) arg;
    FcitxInstanceSetContext(libpinyin->owner->owner, CONTEXT_IM_KEYBOARD_LAYOUT, "us");
    if (libpinyin->type == LPT_Zhuyin) {
        FcitxInstanceSetContext(libpinyin->owner->owner, CONTEXT_ALTERNATIVE_PREVPAGE_KEY, libpinyin->owner->config.hkPrevPage);
        FcitxInstanceSetContext(libpinyin->owner->owner, CONTEXT_ALTERNATIVE_NEXTPAGE_KEY, libpinyin->owner->config.hkNextPage);
    }

    FcitxLibpinyinLoad(libpinyin);

    return true;
}

void FcitxLibpinyinUpdatePreedit(FcitxLibpinyin* libpinyin, char* sentence)
{
    FcitxInstance* instance = libpinyin->owner->owner;
    FcitxInputState* input = FcitxInstanceGetInputState(instance);
    int offset = LibpinyinGetOffset(libpinyin);

    int libpinyinLen = strlen(libpinyin->inst->m_raw_full_pinyin);
    int fcitxLen = strlen(libpinyin->buf);
    if (fcitxLen != libpinyinLen) {
        strcpy(libpinyin->buf, libpinyin->inst->m_raw_full_pinyin);
        libpinyin->cursor_pos += libpinyinLen - fcitxLen;
    }


    int pyoffset = LibpinyinGetPinyinOffset(libpinyin);
    if (pyoffset > libpinyin->cursor_pos)
        libpinyin->cursor_pos = pyoffset;

    int hzlen = 0;
    if (fcitx_utf8_strlen(sentence) > offset)
        hzlen = fcitx_utf8_get_nth_char(sentence, offset) - sentence;
    else
        hzlen = strlen(sentence);

    if (hzlen > 0) {
        char* buf = (char*) fcitx_utils_malloc0((hzlen + 1) * sizeof(char));
        strncpy(buf, sentence, hzlen);
        buf[hzlen] = 0;
        FcitxMessagesAddMessageAtLast(FcitxInputStateGetPreedit(input), MSG_INPUT, "%s", buf);
        free(buf);
    }

    int charcurpos = hzlen;

    int lastpos = pyoffset;
    int curoffset = pyoffset;
    for (int i = offset; i < libpinyin->inst->m_pinyin_keys->len; i ++)
    {
        PinyinKey* pykey = &g_array_index(libpinyin->inst->m_pinyin_keys, PinyinKey, i);
        PinyinKeyPos* pykeypos = &g_array_index(libpinyin->inst->m_pinyin_key_rests, PinyinKeyPos, i);

        if (lastpos > 0) {
            FcitxMessagesMessageConcatLast (FcitxInputStateGetPreedit(input), " ");
            if (curoffset < libpinyin->cursor_pos)
                charcurpos ++;
            for (int j = lastpos; j < pykeypos->m_raw_begin; j ++) {
                char temp[2] = {'\0', '\0'};
                temp[0] = libpinyin->buf[j];
                FcitxMessagesMessageConcatLast (FcitxInputStateGetPreedit(input), temp);
                if (curoffset < libpinyin->cursor_pos)
                {
                    curoffset ++;
                    charcurpos ++;
                }
            }
        }
        lastpos = pykeypos->m_raw_end;

        switch (libpinyin->type) {
            case LPT_Pinyin: {
                gchar* pystring = pykey->get_pinyin_string();
                FcitxMessagesAddMessageAtLast(FcitxInputStateGetPreedit(input), MSG_CODE, "%s", pystring);
                size_t pylen = strlen(pystring);
                if (curoffset + pylen < libpinyin->cursor_pos) {
                    curoffset += pylen;
                    charcurpos += pylen;
                }
                else {
                    charcurpos += libpinyin->cursor_pos - curoffset;
                    curoffset = libpinyin->cursor_pos;
                }
                g_free(pystring);
                break;
            }
            case LPT_Shuangpin: {
                if (pykeypos->length() == 2) {
                    const char* initial = 0;
                    if (pykey->m_initial == CHEWING_ZERO_INITIAL)
                        initial = "'";
                    else
                        initial = get_initial_string(pykey);
                    if (curoffset + 1 <= libpinyin->cursor_pos) {
                        curoffset += 1;
                        charcurpos += strlen(initial);
                    }
                    FcitxMessagesAddMessageAtLast(FcitxInputStateGetPreedit(input), MSG_CODE, "%s", initial);

                    if (curoffset + 1 <= libpinyin->cursor_pos) {
                        curoffset += 1;
                        charcurpos += strlen(get_middle_string(pykey)) + strlen(get_final_string(pykey));
                    }
                    FcitxMessagesAddMessageAtLast(FcitxInputStateGetPreedit(input), MSG_CODE, "%s%s", get_middle_string(pykey), get_final_string(pykey));
                }
                else if (pykeypos->length() == 1) {
                    gchar* pystring = pykey->get_pinyin_string();
                    if (curoffset + 1 <= libpinyin->cursor_pos) {
                        curoffset += 1;
                        charcurpos += strlen(pystring);
                    }
                    FcitxMessagesAddMessageAtLast(FcitxInputStateGetPreedit(input), MSG_CODE, "%s", pystring);
                    g_free(pystring);
                }
                break;
            }
            case LPT_Zhuyin: {
                gchar* pystring = pykey->get_chewing_string();
                FcitxMessagesAddMessageAtLast(FcitxInputStateGetPreedit(input), MSG_CODE, "%s", pystring);

                if (curoffset + pykeypos->length() <= libpinyin->cursor_pos) {
                    curoffset += pykeypos->length();
                    charcurpos += strlen(pystring);
                }
                else {
                    int diff = libpinyin->cursor_pos - curoffset;
                    curoffset = libpinyin->cursor_pos;
                    size_t len = fcitx_utf8_strlen(pystring);
                    if (pykey->m_tone != CHEWING_ZERO_TONE)
                        len --;

                    if (diff > len)
                        charcurpos += strlen(pystring);
                    else {
                        charcurpos += fcitx_utf8_get_nth_char(pystring, diff) - pystring;
                    }
                }
                g_free(pystring);
                break;
            }
        }
    }

    int buflen = strlen(libpinyin->buf);

    if (lastpos < buflen) {
        FcitxMessagesMessageConcatLast (FcitxInputStateGetPreedit(input), " ");
        if (lastpos < libpinyin->cursor_pos)
            charcurpos ++;

        for (int i = lastpos; i < buflen; i ++)
        {
            char temp[2] = {'\0', '\0'};
            temp[0] = libpinyin->buf[i];
            FcitxMessagesMessageConcatLast (FcitxInputStateGetPreedit(input), temp);
            if (lastpos < libpinyin->cursor_pos) {
                charcurpos ++;
                lastpos++;
            }
        }
    }
    FcitxInputStateSetCursorPos(input, charcurpos);
}

inline char* LibPinyinTransString(FcitxLibpinyin* libpinyin, char* sentence)
{
    if (sentence == NULL)
        return NULL;

    return sentence;
}

char* LibpinyinGetSentence(FcitxLibpinyin* libpinyin)
{
    char* sentence = NULL;
    pinyin_get_sentence(libpinyin->inst, &sentence);

    return LibPinyinTransString(libpinyin, sentence);
}

char* LibpinyinTransToken(FcitxLibpinyin* libpinyin, lookup_candidate_t* token)
{
    char* sentence = NULL;
    pinyin_translate_token(libpinyin->inst, token->m_token, &sentence);
    return LibPinyinTransString(libpinyin, sentence);
}

/**
 * @brief function DoInput has done everything for us.
 *
 * @param searchMode
 * @return INPUT_RETURN_VALUE
 **/
__EXPORT_API
INPUT_RETURN_VALUE FcitxLibpinyinGetCandWords(void* arg)
{
    FcitxLibpinyin* libpinyin = (FcitxLibpinyin* )arg;
    FcitxInstance* instance = libpinyin->owner->owner;
    FcitxInputState* input = FcitxInstanceGetInputState(instance);
    FcitxGlobalConfig* config = FcitxInstanceGetGlobalConfig(libpinyin->owner->owner);
    FcitxLibpinyinConfig* pyConfig = &libpinyin->owner->config;
    struct _FcitxCandidateWordList* candList = FcitxInputStateGetCandidateList(input);
    FcitxCandidateWordSetPageSize(candList, config->iMaxCandWord);
    FcitxUICloseInputWindow(instance);
    strcpy(FcitxInputStateGetRawInputBuffer(input), libpinyin->buf);
    FcitxInputStateSetRawInputBufferSize(input, strlen(libpinyin->buf));
    FcitxInputStateSetShowCursor(input, true);
    FcitxInputStateSetClientCursorPos(input, 0);

    if (libpinyin->type == LPT_Zhuyin) {
        FcitxKeyState state = candidateModifierMap[pyConfig->candidateModifiers];
        FcitxCandidateWordSetChooseAndModifier(candList, "1234567890", state);
    }
    else
        FcitxCandidateWordSetChoose(candList, "1234567890");

    /* add punc */
    if (libpinyin->type == LPT_Zhuyin
        && strlen(libpinyin->buf) == 1
        && LibpinyinCheckZhuyinKey((FcitxKeySym) libpinyin->buf[0], pyConfig->zhuyinLayout, pyConfig->useTone)
        && (libpinyin->buf[0] >= ' ' && libpinyin->buf[0] <= '\x7e') /* simple */
        && !(libpinyin->buf[0] >= 'a' && libpinyin->buf[0] <= 'z') /* not a-z */
        && !(libpinyin->buf[0] >= 'A' && libpinyin->buf[0] <= 'Z') /* not A-Z /*/
        && !(libpinyin->buf[0] >= '0' && libpinyin->buf[0] <= '9') /* not digit */
    ) {
        FcitxModuleFunctionArg arg;
        int c = libpinyin->buf[0];
        arg.args[0] = &c;
        char* result = InvokeFunction(instance, FCITX_PUNC, GETPUNC, arg);
        if (result) {
            FcitxCandidateWord candWord;
            FcitxLibpinyinCandWord* pyCand = (FcitxLibpinyinCandWord*) fcitx_utils_malloc0(sizeof(FcitxLibpinyinCandWord));
            pyCand->issentence = false;
            pyCand->ispunc = true;
            candWord.callback = FcitxLibpinyinGetCandWord;
            candWord.extraType = MSG_OTHER;
            candWord.owner = libpinyin;
            candWord.priv = pyCand;
            candWord.strExtra = NULL;
            candWord.strWord = strdup(result);
            candWord.wordType = MSG_OTHER;

            FcitxCandidateWordAppend(FcitxInputStateGetCandidateList(input), &candWord);
        }
    }
    char* sentence = NULL;

    pinyin_guess_sentence(libpinyin->inst);
    sentence = LibpinyinGetSentence(libpinyin);
    if (sentence) {
        FcitxCandidateWord candWord;
        FcitxLibpinyinCandWord* pyCand = (FcitxLibpinyinCandWord*) fcitx_utils_malloc0(sizeof(FcitxLibpinyinCandWord));
        pyCand->issentence = true;
        pyCand->ispunc = false;
        candWord.callback = FcitxLibpinyinGetCandWord;
        candWord.extraType = MSG_OTHER;
        candWord.owner = libpinyin;
        candWord.priv = pyCand;
        candWord.strExtra = NULL;
        candWord.strWord = strdup(sentence);
        candWord.wordType = MSG_OTHER;

        FcitxCandidateWordAppend(FcitxInputStateGetCandidateList(input), &candWord);

        FcitxLibpinyinUpdatePreedit(libpinyin, sentence);

        FcitxMessagesAddMessageAtLast(FcitxInputStateGetClientPreedit(input), MSG_INPUT, "%s", sentence);
    }
    else {
        FcitxInputStateSetCursorPos(input, libpinyin->cursor_pos);
        FcitxMessagesAddMessageAtLast(FcitxInputStateGetClientPreedit(input), MSG_INPUT, "%s", libpinyin->buf);
        FcitxMessagesAddMessageAtLast(FcitxInputStateGetPreedit(input), MSG_INPUT, "%s", libpinyin->buf);
    }

    CandidateVector array = g_array_new(FALSE, FALSE, sizeof(lookup_candidate_t));
    pinyin_get_full_pinyin_candidates(libpinyin->inst, LibpinyinGetOffset(libpinyin), array);
    int i = 0;
    for (i = 0 ; i < array->len; i ++) {
        lookup_candidate_t token = g_array_index(array, lookup_candidate_t, i);
        char* tokenstring = LibpinyinTransToken(libpinyin, &token);

        if (tokenstring) {
            if (sentence && strcmp(sentence, tokenstring) == 0)
                continue;

            FcitxCandidateWord candWord;
            FcitxLibpinyinCandWord* pyCand = (FcitxLibpinyinCandWord*) fcitx_utils_malloc0(sizeof(FcitxLibpinyinCandWord));
            pyCand->issentence = false;
            pyCand->ispunc = false;
            pyCand->token = token;
            candWord.callback = FcitxLibpinyinGetCandWord;
            candWord.extraType = MSG_OTHER;
            candWord.owner = libpinyin;
            candWord.priv = pyCand;
            candWord.strExtra = NULL;
            candWord.strWord = strdup(tokenstring);
            candWord.wordType = MSG_OTHER;

            FcitxCandidateWordAppend(FcitxInputStateGetCandidateList(input), &candWord);
        }

        g_free(tokenstring);
    }

    g_array_free(array, TRUE);

    g_free(sentence);

    return IRV_DISPLAY_CANDWORDS;
}

/**
 * @brief get the candidate word by index
 *
 * @param iIndex index of candidate word
 * @return the string of canidate word
 **/
__EXPORT_API
INPUT_RETURN_VALUE FcitxLibpinyinGetCandWord (void* arg, FcitxCandidateWord* candWord)
{
    FcitxLibpinyin* libpinyin = (FcitxLibpinyin* )arg;
    FcitxLibpinyinCandWord* pyCand = (FcitxLibpinyinCandWord*) candWord->priv;
    FcitxInstance* instance = libpinyin->owner->owner;
    FcitxInputState* input = FcitxInstanceGetInputState(instance);

    if (pyCand->issentence) {
        char* sentence = LibpinyinGetSentence(libpinyin);
        if (sentence) {
            strcpy(FcitxInputStateGetOutputString(input), sentence);
            pinyin_train(libpinyin->inst);
        }
        else
            strcpy(FcitxInputStateGetOutputString(input), "");

        return IRV_COMMIT_STRING;
    } else if (pyCand->ispunc) {
        strcpy(FcitxInputStateGetOutputString(input), candWord->strWord);
        return IRV_COMMIT_STRING;
    } else {
        pinyin_choose_full_pinyin_candidate(libpinyin->inst, LibpinyinGetOffset(libpinyin), &pyCand->token);
        char* tokenstring = LibpinyinTransToken(libpinyin, &pyCand->token);

        if (tokenstring) {
            FcitxLibpinyinFixed f;
            f.len = fcitx_utf8_strlen(tokenstring);
            f.token = pyCand->token;
            g_array_append_val(libpinyin->fixed_string, f);
        }

        g_free(tokenstring);

        int offset = LibpinyinGetOffset(libpinyin);
        if (offset >= libpinyin->inst->m_pinyin_keys->len)
        {
            char* sentence = NULL;
            pinyin_guess_sentence(libpinyin->inst);
            sentence = LibpinyinGetSentence(libpinyin);
            if (sentence) {
                strcpy(FcitxInputStateGetOutputString(input), sentence);
                pinyin_train(libpinyin->inst);
            }
            else
                strcpy(FcitxInputStateGetOutputString(input), "");

            return IRV_COMMIT_STRING;
        }

        int pyoffset = LibpinyinGetPinyinOffset(libpinyin);
        if (pyoffset > libpinyin->cursor_pos)
            libpinyin->cursor_pos = pyoffset;

        return IRV_DISPLAY_CANDWORDS;
    }
    return IRV_TO_PROCESS;
}

FcitxLibpinyin* FcitxLibpinyinNew(FcitxLibpinyinAddonInstance* libpinyinaddon, LIBPINYIN_TYPE type)
{
    FcitxLibpinyin* libpinyin = (FcitxLibpinyin*) fcitx_utils_malloc0(sizeof(FcitxLibpinyin));
    libpinyin->inst = NULL;
    libpinyin->fixed_string = g_array_new(FALSE, FALSE, sizeof(FcitxLibpinyinFixed));
    libpinyin->type = type;
    libpinyin->owner = libpinyinaddon;
    return libpinyin;
}

void FcitxLibpinyinDelete(FcitxLibpinyin* libpinyin)
{
    if (libpinyin->inst)
        pinyin_free_instance(libpinyin->inst);
    g_array_free(libpinyin->fixed_string, TRUE);
}

/**
 * @brief initialize the extra input method
 *
 * @param arg
 * @return successful or not
 **/
__EXPORT_API
void* FcitxLibpinyinCreate (FcitxInstance* instance)
{
    FcitxLibpinyinAddonInstance* libpinyinaddon = (FcitxLibpinyinAddonInstance*) fcitx_utils_malloc0(sizeof(FcitxLibpinyinAddonInstance));
    bindtextdomain("fcitx-libpinyin", LOCALEDIR);
    libpinyinaddon->owner = instance;

    if (!LoadLibpinyinConfig(&libpinyinaddon->config))
    {
        free(libpinyinaddon);
        return NULL;
    }

    libpinyinaddon->pinyin = FcitxLibpinyinNew(libpinyinaddon, LPT_Pinyin);
    libpinyinaddon->shuangpin = FcitxLibpinyinNew(libpinyinaddon, LPT_Shuangpin);
    libpinyinaddon->zhuyin = FcitxLibpinyinNew(libpinyinaddon, LPT_Zhuyin);

    ConfigLibpinyin(libpinyinaddon);

    FcitxInstanceRegisterIM(instance,
                    libpinyinaddon->pinyin,
                    "pinyin-libpinyin",
                    _("Pinyin (LibPinyin)"),
                    "pinyin",
                    FcitxLibpinyinInit,
                    FcitxLibpinyinReset,
                    FcitxLibpinyinDoInput,
                    FcitxLibpinyinGetCandWords,
                    NULL,
                    FcitxLibpinyinSave,
                    ReloadConfigFcitxLibpinyin,
                    NULL,
                    5,
                    libpinyinaddon->config.bTraditionalDataForPinyin ? "zh_TW" : "zh_CN"
                   );

    FcitxInstanceRegisterIM(instance,
                    libpinyinaddon->shuangpin,
                    "shuangpin-libpinyin",
                    _("Shuangpin (LibPinyin)"),
                    "shuangpin",
                    FcitxLibpinyinInit,
                    FcitxLibpinyinReset,
                    FcitxLibpinyinDoInput,
                    FcitxLibpinyinGetCandWords,
                    NULL,
                    FcitxLibpinyinSave,
                    ReloadConfigFcitxLibpinyin,
                    NULL,
                    5,
                    libpinyinaddon->config.bTraditionalDataForPinyin ? "zh_TW" : "zh_CN"
                   );

    FcitxInstanceRegisterIM(instance,
                    libpinyinaddon->zhuyin,
                    "zhuyin-libpinyin",
                    _("Bopomofo (LibPinyin)"),
                    "bopomofo",
                    FcitxLibpinyinInit,
                    FcitxLibpinyinReset,
                    FcitxLibpinyinDoInput,
                    FcitxLibpinyinGetCandWords,
                    NULL,
                    FcitxLibpinyinSave,
                    ReloadConfigFcitxLibpinyin,
                    NULL,
                    5,
                    libpinyinaddon->config.bSimplifiedDataForZhuyin ? "zh_CN" : "zh_TW"
                   );

    return libpinyinaddon;
}

/**
 * @brief Destroy the input method while unload it.
 *
 * @return int
 **/
__EXPORT_API
void FcitxLibpinyinDestroy (void* arg)
{
    FcitxLibpinyinAddonInstance* libpinyin = (FcitxLibpinyinAddonInstance*) arg;
    FcitxLibpinyinDelete(libpinyin->pinyin);
    FcitxLibpinyinDelete(libpinyin->zhuyin);
    FcitxLibpinyinDelete(libpinyin->shuangpin);
    if (libpinyin->pinyin_context)
        pinyin_fini(libpinyin->pinyin_context);
    if (libpinyin->zhuyin_context)
        pinyin_fini(libpinyin->zhuyin_context);
}

/**
 * @brief Load the config file for fcitx-libpinyin
 *
 * @param Bool is reload or not
 **/
boolean LoadLibpinyinConfig(FcitxLibpinyinConfig* fs)
{
    FcitxConfigFileDesc *configDesc = GetLibpinyinConfigDesc();
    if (!configDesc)
        return false;

    FILE *fp = FcitxXDGGetFileUserWithPrefix("conf", "fcitx-libpinyin.config", "r", NULL);

    if (!fp)
    {
        if (errno == ENOENT)
            SaveLibpinyinConfig(fs);
    }
    FcitxConfigFile *cfile = FcitxConfigParseConfigFileFp(fp, configDesc);

    FcitxLibpinyinConfigConfigBind(fs, cfile, configDesc);
    FcitxConfigBindSync(&fs->gconfig);

    if (fp)
        fclose(fp);
    return true;
}

void ConfigLibpinyin(FcitxLibpinyinAddonInstance* libpinyinaddon)
{
    FcitxLibpinyinConfig *config = &libpinyinaddon->config;
    if (libpinyinaddon->zhuyin_context)
        pinyin_set_chewing_scheme(libpinyinaddon->zhuyin_context, FcitxLibpinyinTransZhuyinLayout(config->zhuyinLayout));
    if (libpinyinaddon->pinyin_context)
        pinyin_set_double_pinyin_scheme(libpinyinaddon->pinyin_context, FcitxLibpinyinTransShuangpinScheme(config->spScheme));
    pinyin::pinyin_option_t settings = 0;
    settings |= DYNAMIC_ADJUST;
    settings |= USE_DIVIDED_TABLE | USE_RESPLIT_TABLE;
    for (int i = 0; i <= FCITX_CR_LAST; i ++) {
        if (config->cr[i])
            settings |= FcitxLibpinyinTransCorrection((FCITX_CORRECTION) i);
    }
    for (int i = 0; i <= FCITX_AMB_LAST; i ++) {
        if (config->amb[i])
            settings |= FcitxLibpinyinTransAmbiguity((FCITX_AMBIGUITY) i);
    }

    if (config->incomplete) {
        settings |= PINYIN_INCOMPLETE;
    }

    if (config->chewingIncomplete) {
        settings |= CHEWING_INCOMPLETE;
    }

    if (config->useTone) {
        settings |= USE_TONE;
    }
    settings |= IS_PINYIN;
    settings |= IS_CHEWING;
    if (libpinyinaddon->pinyin_context)
        pinyin_set_options(libpinyinaddon->pinyin_context, settings);
    if (libpinyinaddon->zhuyin_context)
        pinyin_set_options(libpinyinaddon->zhuyin_context, settings);
}

__EXPORT_API void ReloadConfigFcitxLibpinyin(void* arg)
{
    FcitxLibpinyin* libpinyin = (FcitxLibpinyin*) arg;
    FcitxLibpinyinAddonInstance* libpinyinaddon = libpinyin->owner;
    LoadLibpinyinConfig(&libpinyinaddon->config);
    ConfigLibpinyin(libpinyinaddon);
}

/**
 * @brief Save the config
 *
 * @return void
 **/
void SaveLibpinyinConfig(FcitxLibpinyinConfig* fs)
{
    FcitxConfigFileDesc *configDesc = GetLibpinyinConfigDesc();
    FILE *fp = FcitxXDGGetFileUserWithPrefix("conf", "fcitx-libpinyin.config", "w", NULL);
    FcitxConfigSaveConfigFileFp(fp, &fs->gconfig, configDesc);
    if (fp)
        fclose(fp);
}

void FcitxLibpinyinSave(void* arg)
{
    FcitxLibpinyin* libpinyin = (FcitxLibpinyin*) arg;
    if (libpinyin->owner->zhuyin_context)
        pinyin_save(libpinyin->owner->zhuyin_context);
    if (libpinyin->owner->pinyin_context)
        pinyin_save(libpinyin->owner->pinyin_context);
}


// kate: indent-mode cstyle; space-indent on; indent-width 0;
