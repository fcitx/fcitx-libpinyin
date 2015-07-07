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

#include <sstream>
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
#include <fcitx/module/punc/fcitx-punc.h>
#include <string>
#include <libintl.h>

#include "config.h"
#include "eim.h"
#include "enummap.h"
#include "bus.h"
#include "common.h"
#include "utils.h"

#define FCITX_LIBPINYIN_MAX(x, y) ((x) > (y)? (x) : (y))
#define FCITX_LIBPINYIN_MIN(x, y) ((x) < (y)? (x) : (y))
extern "C" {
    FCITX_DEFINE_PLUGIN(fcitx_libpinyin, ime2, FcitxIMClass2) = {
        FcitxLibPinyinCreate,
        FcitxLibPinyinDestroy,
        FcitxLibPinyinReloadConfig,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
    };
}

typedef struct _FcitxLibPinyinCandWord {
    boolean ispunc;
    int idx;
} FcitxLibPinyinCandWord;

typedef struct _FcitxLibPinyinFixed {
    int len;
} FcitxLibPinyinFixed;

CONFIG_DEFINE_LOAD_AND_SAVE(FcitxLibPinyinConfig, FcitxLibPinyinConfig, "fcitx-libpinyin")

static void FcitxLibPinyinReconfigure(FcitxLibPinyinAddonInstance* libpinyin);
static int LibPinyinGetOffset(FcitxLibPinyin* libpinyin);
static void FcitxLibPinyinSave(void* arg);
static bool LibPinyinCheckZhuyinKey(FcitxKeySym sym, FCITX_ZHUYIN_LAYOUT layout, boolean useTone) ;
char* LibPinyinGetSentence(FcitxLibPinyin* libpinyin);

static const char* input_keys [] = {
    "1qaz2wsxedcrfv5tgbyhnujm8ik,9ol.0p;/-",       /* standard kb */
    "1234567890-qwertyuiopasdfghjkl;zxcvbn",       /* IBM */
    "2wsx3edcrfvtgb6yhnujm8ik,9ol.0p;/-['=",       /* Gin-yieh */
    "bpmfdtnlvkhg7c,./j;'sexuaorwiqzy890-=",       /* ET  */
    0
};

static const char* tone_keys [] = {
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

bool LibPinyinCheckZhuyinKey(FcitxKeySym sym, FCITX_ZHUYIN_LAYOUT layout, boolean useTone)
{
    char key = sym & 0xff;
    const char* keys = input_keys[layout];
    const char* tones = tone_keys[layout];
    if (strchr(keys, key))
        return true;

    if (useTone && strchr(tones, key))
        return true;

    return false;
}

int LibPinyinGetOffset(FcitxLibPinyin* libpinyin)
{
    GArray* array = libpinyin->fixed_string;
    int sum = 0;
    for (int i = 0; i < array->len; i ++) {
        FcitxLibPinyinFixed* f = &g_array_index(array, FcitxLibPinyinFixed, i);
        sum += f->len;
    }
    return sum;
}

int LibPinyinGetPinyinOffset(FcitxLibPinyin* libpinyin)
{
    int offset = LibPinyinGetOffset(libpinyin);
    guint16 pyoffset = 0;

    guint len;

    pinyin_get_n_pinyin(libpinyin->inst, &len);

    int i = FCITX_LIBPINYIN_MIN(offset, len) - 1;
    if (i >= 0) {
        PinyinKeyPos* pykey = NULL;
        pinyin_get_pinyin_key_rest(libpinyin->inst, i, &pykey);
        pinyin_get_pinyin_key_rest_positions(libpinyin->inst, pykey, NULL, &pyoffset);
    }

    return pyoffset;
}

/**
 * @brief Reset the status.
 *
 **/
void FcitxLibPinyinReset(void* arg)
{
    FcitxLibPinyin* libpinyin = (FcitxLibPinyin*) arg;
    libpinyin->buf[0] = '\0';
    libpinyin->cursor_pos = 0;

    if (libpinyin->fixed_string->len > 0)
        g_array_remove_range(libpinyin->fixed_string, 0, libpinyin->fixed_string->len);
    if (libpinyin->inst)
        pinyin_reset(libpinyin->inst);
}

size_t FcitxLibPinyinParse(FcitxLibPinyin* libpinyin, const char* str)
{
    switch (libpinyin->type) {
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
INPUT_RETURN_VALUE FcitxLibPinyinDoInput(void* arg, FcitxKeySym sym, unsigned int state)
{
    FcitxLibPinyin* libpinyin = (FcitxLibPinyin*) arg;
    FcitxLibPinyinConfig* config = &libpinyin->owner->config;
    FcitxInstance* instance = libpinyin->owner->owner;
    FcitxInputState* input = FcitxInstanceGetInputState(libpinyin->owner->owner);

    if (FcitxHotkeyIsHotKeySimple(sym, state)) {
        /* there is some special case that ';' is used */
        if (FcitxHotkeyIsHotKeyLAZ(sym, state)
                || sym == '\''
                || (FcitxHotkeyIsHotKey(sym, state, FCITX_SEMICOLON) && libpinyin->type == LPT_Shuangpin && (config->spScheme == FCITX_SHUANG_PIN_MS || config->spScheme == FCITX_SHUANG_PIN_ZIGUANG))
                || (libpinyin->type == LPT_Zhuyin && LibPinyinCheckZhuyinKey(sym, config->zhuyinLayout, config->useTone))
           ) {
            if (strlen(libpinyin->buf) == 0 && (sym == '\'' || sym == ';'))
                return IRV_TO_PROCESS;

            if (strlen(libpinyin->buf) < MAX_PINYIN_INPUT) {
                size_t len = strlen(libpinyin->buf);
                if (libpinyin->buf[libpinyin->cursor_pos] != 0) {
                    memmove(libpinyin->buf + libpinyin->cursor_pos + 1, libpinyin->buf + libpinyin->cursor_pos, len - libpinyin->cursor_pos);
                }
                libpinyin->buf[len + 1] = 0;
                libpinyin->buf[libpinyin->cursor_pos] = (char)(sym & 0xff);
                libpinyin->cursor_pos ++;

                size_t parselen = FcitxLibPinyinParse(libpinyin, libpinyin->buf);

                if (parselen == 0 && strlen(libpinyin->buf) == 1 && libpinyin->type != LPT_Shuangpin
                        && !(libpinyin->type == LPT_Pinyin && !libpinyin->owner->config.incomplete)
                        && !(libpinyin->type == LPT_Zhuyin && !libpinyin->owner->config.chewingIncomplete)) {
                    FcitxLibPinyinReset(libpinyin);
                    return IRV_TO_PROCESS;
                }
                return IRV_DISPLAY_CANDWORDS;
            } else
                return IRV_DO_NOTHING;
        }
    }

    if (FcitxHotkeyIsHotKey(sym, state, FCITX_SPACE)
            || (libpinyin->type == LPT_Zhuyin && FcitxHotkeyIsHotKey(sym, state, FCITX_ENTER))) {
        size_t len = strlen(libpinyin->buf);
        if (len == 0)
            return IRV_TO_PROCESS;

        return FcitxCandidateWordChooseByIndex(FcitxInputStateGetCandidateList(input), 0);
    }

    if ((libpinyin->type != LPT_Zhuyin && FcitxHotkeyIsHotKey(sym, state, FCITX_ENTER))
        || (libpinyin->type == LPT_Zhuyin && FcitxHotkeyIsHotKey(sym, state, FCITX_LIBPINYIN_SHIFT_ENTER))) {
        if (libpinyin->buf[0] == 0)
            return IRV_TO_PROCESS;

        char* sentence = LibPinyinGetSentence(libpinyin);
        if (sentence) {
            int offset = LibPinyinGetOffset(libpinyin);
            int hzlen = 0;
            if (fcitx_utf8_strlen(sentence) > offset) {
                hzlen = fcitx_utf8_get_nth_char(sentence, offset) - sentence;
            } else {
                hzlen = strlen(sentence);
            }

            int start = LibPinyinGetPinyinOffset(libpinyin);
            int pylen = strlen(libpinyin->buf) - start;

            if (pylen < 0) {
                pylen = 0;
            }

            char* buf = (char*) fcitx_utils_malloc0((hzlen + pylen + 1) * sizeof(char));
            strncpy(buf, sentence, hzlen);
            if (pylen) {
                strcpy(&buf[hzlen], &libpinyin->buf[start]);
            }
            buf[hzlen + pylen] = '\0';
            FcitxInstanceCommitString(instance, FcitxInstanceGetCurrentIC(instance), buf);
            g_free(sentence);
            free(buf);
        } else {
            FcitxInstanceCommitString(instance, FcitxInstanceGetCurrentIC(instance), libpinyin->buf);
        }

        return IRV_CLEAN;
    }

    if (FcitxHotkeyIsHotKey(sym, state, FCITX_BACKSPACE) || FcitxHotkeyIsHotKey(sym, state, FCITX_DELETE)) {
        if (strlen(libpinyin->buf) > 0) {
            int offset = LibPinyinGetOffset(libpinyin);
            if (offset != 0 && FcitxHotkeyIsHotKey(sym, state, FCITX_BACKSPACE)) {
                g_array_remove_index_fast(libpinyin->fixed_string, libpinyin->fixed_string->len - 1);
                pinyin_clear_constraint(libpinyin->inst, LibPinyinGetOffset(libpinyin));
            } else {
                if (FcitxHotkeyIsHotKey(sym, state, FCITX_BACKSPACE)) {
                    if (libpinyin->cursor_pos > 0) {
                        libpinyin->cursor_pos -- ;
                    } else {
                        return IRV_DO_NOTHING;
                    }
                }
                size_t len = strlen(libpinyin->buf);
                if (libpinyin->cursor_pos == (int)len)
                    return IRV_DO_NOTHING;
                memmove(libpinyin->buf + libpinyin->cursor_pos, libpinyin->buf + libpinyin->cursor_pos + 1, len - libpinyin->cursor_pos - 1);
                libpinyin->buf[strlen(libpinyin->buf) - 1] = 0;
                if (libpinyin->buf[0] == '\0')
                    return IRV_CLEAN;
                else
                    FcitxLibPinyinParse(libpinyin, libpinyin->buf);
            }
            return IRV_DISPLAY_CANDWORDS;
        } else
            return IRV_TO_PROCESS;
    } else {
        if (strlen(libpinyin->buf) > 0) {
            if (FcitxHotkeyIsHotKey(sym, state, FCITX_LEFT)) {
                if (libpinyin->cursor_pos > 0) {
                    if (libpinyin->cursor_pos == LibPinyinGetPinyinOffset(libpinyin)) {
                        g_array_remove_index_fast(libpinyin->fixed_string, libpinyin->fixed_string->len - 1);
                        pinyin_clear_constraint(libpinyin->inst, LibPinyinGetOffset(libpinyin));
                        return IRV_DISPLAY_CANDWORDS;
                    } else {
                        libpinyin->cursor_pos--;
                        return IRV_DISPLAY_CANDWORDS;
                    }
                }

                return IRV_DO_NOTHING;
            } else if (FcitxHotkeyIsHotKey(sym, state, FCITX_RIGHT)) {
                size_t len = strlen(libpinyin->buf);
                if (libpinyin->cursor_pos < (int) len) {
                    libpinyin->cursor_pos ++ ;
                    return IRV_DISPLAY_CANDWORDS;
                }
                return IRV_DO_NOTHING;
            } else if (FcitxHotkeyIsHotKey(sym, state, FCITX_HOME)) {
                int offset = LibPinyinGetPinyinOffset(libpinyin);
                if (libpinyin->cursor_pos != offset) {
                    libpinyin->cursor_pos = offset;
                    return IRV_DISPLAY_CANDWORDS;
                }
                return IRV_DO_NOTHING;
            } else if (FcitxHotkeyIsHotKey(sym, state, FCITX_END)) {
                size_t len = strlen(libpinyin->buf);
                if (libpinyin->cursor_pos != (int) len) {
                    libpinyin->cursor_pos = len ;
                    return IRV_DISPLAY_CANDWORDS;
                }
                return IRV_DO_NOTHING;
            }
        } else {
            return IRV_TO_PROCESS;
        }
    }
    return IRV_TO_PROCESS;
}

void FcitxLibPinyinLoad(FcitxLibPinyin* libpinyin)
{
    if (libpinyin->inst != NULL)
        return;

    FcitxLibPinyinAddonInstance* libpinyinaddon = libpinyin->owner;

    if (libpinyin->type == LPT_Zhuyin && libpinyin->owner->zhuyin_context == NULL) {
        char* user_path = FcitxLibPinyinGetUserPath(libpinyinaddon->config.bSimplifiedDataForZhuyin ? LPLT_Simplified : LPLT_Traditional);
        char* syspath = FcitxLibPinyinGetSysPath(libpinyinaddon->config.bSimplifiedDataForZhuyin ? LPLT_Simplified : LPLT_Traditional);
        libpinyinaddon->zhuyin_context = pinyin_init(syspath, user_path);/*
        pinyin_load_phrase_library(libpinyinaddon->zhuyin_context, GB_DICTIONARY);
        pinyin_load_phrase_library(libpinyinaddon->zhuyin_context, GBK_DICTIONARY);
        pinyin_load_phrase_library(libpinyinaddon->zhuyin_context, ADDON_DICTIONARY);
        pinyin_load_phrase_library(libpinyinaddon->zhuyin_context, USER_DICTIONARY);
        pinyin_load_phrase_library(libpinyinaddon->zhuyin_context, NETWORK_DICTIONARY);*/
        free(user_path);
        free(syspath);
    }

    if (libpinyin->type != LPT_Zhuyin && libpinyin->owner->pinyin_context == NULL) {
        char* user_path = FcitxLibPinyinGetUserPath(libpinyinaddon->config.bTraditionalDataForPinyin ? LPLT_Traditional : LPLT_Simplified);
        char* syspath = FcitxLibPinyinGetSysPath(libpinyinaddon->config.bTraditionalDataForPinyin ? LPLT_Traditional : LPLT_Simplified);
        libpinyinaddon->pinyin_context = pinyin_init(syspath, user_path);/*
        pinyin_load_phrase_library(libpinyinaddon->pinyin_context, GB_DICTIONARY);
        pinyin_load_phrase_library(libpinyinaddon->pinyin_context, GBK_DICTIONARY);
        pinyin_load_phrase_library(libpinyinaddon->pinyin_context, ADDON_DICTIONARY);
        pinyin_load_phrase_library(libpinyinaddon->pinyin_context, USER_DICTIONARY);
        pinyin_load_phrase_library(libpinyinaddon->pinyin_context, NETWORK_DICTIONARY);*/
        free(user_path);
        free(syspath);
    }

    if (libpinyin->type == LPT_Zhuyin)
        libpinyin->inst = pinyin_alloc_instance(libpinyinaddon->zhuyin_context);
    else
        libpinyin->inst = pinyin_alloc_instance(libpinyinaddon->pinyin_context);

    FcitxLibPinyinReconfigure(libpinyinaddon);
}

boolean FcitxLibPinyinInit(void* arg)
{
    FcitxLibPinyin* libpinyin = (FcitxLibPinyin*) arg;
    FcitxInstanceSetContext(libpinyin->owner->owner, CONTEXT_IM_KEYBOARD_LAYOUT, "us");
    if (libpinyin->type == LPT_Zhuyin) {
        FcitxInstanceSetContext(libpinyin->owner->owner, CONTEXT_ALTERNATIVE_PREVPAGE_KEY, libpinyin->owner->config.hkPrevPage);
        FcitxInstanceSetContext(libpinyin->owner->owner, CONTEXT_ALTERNATIVE_NEXTPAGE_KEY, libpinyin->owner->config.hkNextPage);
    }

    FcitxLibPinyinLoad(libpinyin);

    return true;
}

void FcitxLibPinyinUpdatePreedit(FcitxLibPinyin* libpinyin, char* sentence)
{
    FcitxInstance* instance = libpinyin->owner->owner;
    FcitxInputState* input = FcitxInstanceGetInputState(instance);
    int offset = LibPinyinGetOffset(libpinyin);

    if (libpinyin->type == LPT_Pinyin) {
        const gchar* raw_full_pinyin;
        pinyin_get_raw_full_pinyin(libpinyin->inst, &raw_full_pinyin);
        int libpinyinLen = strlen(raw_full_pinyin);
        int fcitxLen = strlen(libpinyin->buf);
        if (fcitxLen != libpinyinLen) {
            strcpy(libpinyin->buf, raw_full_pinyin);
            libpinyin->cursor_pos += libpinyinLen - fcitxLen;
        }
    }

    int pyoffset = LibPinyinGetPinyinOffset(libpinyin);
    if (pyoffset > libpinyin->cursor_pos)
        libpinyin->cursor_pos = pyoffset;

    int hzlen = 0;
    if (fcitx_utf8_strlen(sentence) > offset) {
        hzlen = fcitx_utf8_get_nth_char(sentence, offset) - sentence;
    } else {
        hzlen = strlen(sentence);
    }

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
    guint pinyinLen = 0;
    pinyin_get_n_pinyin(libpinyin->inst, &pinyinLen);
    for (int i = offset; i < pinyinLen; i ++) {
        PinyinKey* pykey = NULL;
        PinyinKeyPos* pykeypos = NULL;
        pinyin_get_pinyin_key(libpinyin->inst, i, &pykey);
        pinyin_get_pinyin_key_rest(libpinyin->inst, i, &pykeypos);

        guint16 rawBegin = 0, rawEnd = 0;
        pinyin_get_pinyin_key_rest_positions(libpinyin->inst, pykeypos, &rawBegin, &rawEnd);
        if (lastpos > 0) {
            FcitxMessagesMessageConcatLast(FcitxInputStateGetPreedit(input), " ");
            if (curoffset < libpinyin->cursor_pos)
                charcurpos ++;
            for (int j = lastpos; j < rawBegin; j ++) {
                char temp[2] = {'\0', '\0'};
                temp[0] = libpinyin->buf[j];
                FcitxMessagesMessageConcatLast(FcitxInputStateGetPreedit(input), temp);
                if (curoffset < libpinyin->cursor_pos) {
                    curoffset ++;
                    charcurpos ++;
                }
            }
        }
        lastpos = rawEnd;

        switch (libpinyin->type) {
        case LPT_Pinyin: {
            gchar* pystring;
            pinyin_get_pinyin_string(libpinyin->inst, pykey, &pystring);
            FcitxMessagesAddMessageAtLast(FcitxInputStateGetPreedit(input), MSG_CODE, "%s", pystring);
            size_t pylen = strlen(pystring);
            if (curoffset + pylen < libpinyin->cursor_pos) {
                curoffset += pylen;
                charcurpos += pylen;
            } else {
                charcurpos += libpinyin->cursor_pos - curoffset;
                curoffset = libpinyin->cursor_pos;
            }
            g_free(pystring);
            break;
        }
        case LPT_Shuangpin: {
            guint16 pykeyposLen = 0;
            pinyin_get_pinyin_key_rest_length(libpinyin->inst, pykeypos, &pykeyposLen);
            if (pykeyposLen == 2) {
                const gchar* initial = NULL;
                gchar* middle_final = NULL, *_initial = NULL;
                pinyin_get_pinyin_strings(libpinyin->inst, pykey, &_initial, &middle_final);
                if (!_initial[0])
                    initial = "'";
                else
                    initial = _initial;
                if (curoffset + 1 <= libpinyin->cursor_pos) {
                    curoffset += 1;
                    charcurpos += strlen(initial);
                }
                FcitxMessagesAddMessageAtLast(FcitxInputStateGetPreedit(input), MSG_CODE, "%s", initial);

                if (curoffset + 1 <= libpinyin->cursor_pos) {
                    curoffset += 1;
                    charcurpos += strlen(middle_final);
                }
                FcitxMessagesAddMessageAtLast(FcitxInputStateGetPreedit(input), MSG_CODE, "%s", middle_final);
                g_free(_initial);
                g_free(middle_final);
            } else if (pykeyposLen == 1) {
                gchar* pystring;
                pinyin_get_pinyin_string(libpinyin->inst, pykey, &pystring);
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
            guint16 pykeyposLen = 0;
            pinyin_get_pinyin_key_rest_length(libpinyin->inst, pykeypos, &pykeyposLen);
            gchar* pystring;
            pinyin_get_chewing_string(libpinyin->inst, pykey, &pystring);
            FcitxMessagesAddMessageAtLast(FcitxInputStateGetPreedit(input), MSG_CODE, "%s", pystring);

            if (curoffset + pykeyposLen <= libpinyin->cursor_pos) {
                curoffset += pykeyposLen;
                charcurpos += strlen(pystring);
            } else {
                int diff = libpinyin->cursor_pos - curoffset;
                curoffset = libpinyin->cursor_pos;
                size_t len = fcitx_utf8_strlen(pystring);

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
        FcitxMessagesMessageConcatLast(FcitxInputStateGetPreedit(input), " ");
        if (lastpos < libpinyin->cursor_pos)
            charcurpos ++;

        for (int i = lastpos; i < buflen; i ++) {
            char temp[2] = {'\0', '\0'};
            temp[0] = libpinyin->buf[i];
            FcitxMessagesMessageConcatLast(FcitxInputStateGetPreedit(input), temp);
            if (lastpos < libpinyin->cursor_pos) {
                charcurpos ++;
                lastpos++;
            }
        }
    }
    FcitxInputStateSetCursorPos(input, charcurpos);
}

char* LibPinyinGetSentence(FcitxLibPinyin* libpinyin)
{
    char* sentence = NULL;
    pinyin_get_sentence(libpinyin->inst, &sentence);

    return sentence;
}

/**
 * @brief function DoInput has done everything for us.
 *
 * @param searchMode
 * @return INPUT_RETURN_VALUE
 **/
INPUT_RETURN_VALUE FcitxLibPinyinGetCandWords(void* arg)
{
    FcitxLibPinyin* libpinyin = (FcitxLibPinyin*)arg;
    FcitxInstance* instance = libpinyin->owner->owner;
    FcitxInputState* input = FcitxInstanceGetInputState(instance);
    FcitxGlobalConfig* config = FcitxInstanceGetGlobalConfig(libpinyin->owner->owner);
    FcitxLibPinyinConfig* pyConfig = &libpinyin->owner->config;
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
    } else
        FcitxCandidateWordSetChoose(candList, "1234567890");

    /* add punc */
    if (libpinyin->type == LPT_Zhuyin
            && strlen(libpinyin->buf) == 1
            && LibPinyinCheckZhuyinKey((FcitxKeySym) libpinyin->buf[0], pyConfig->zhuyinLayout, pyConfig->useTone)
            && (libpinyin->buf[0] >= ' ' && libpinyin->buf[0] <= '\x7e') /* simple */
            && !(libpinyin->buf[0] >= 'a' && libpinyin->buf[0] <= 'z') /* not a-z */
            && !(libpinyin->buf[0] >= 'A' && libpinyin->buf[0] <= 'Z') /* not A-Z /*/
            && !(libpinyin->buf[0] >= '0' && libpinyin->buf[0] <= '9') /* not digit */
       ) {
        int c = libpinyin->buf[0];
        char* result = FcitxPuncGetPunc(instance, &c);
        if (result) {
            FcitxCandidateWord candWord;
            FcitxLibPinyinCandWord* pyCand = (FcitxLibPinyinCandWord*) fcitx_utils_malloc0(sizeof(FcitxLibPinyinCandWord));
            pyCand->ispunc = true;
            candWord.callback = FcitxLibPinyinGetCandWord;
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
    sentence = LibPinyinGetSentence(libpinyin);
    if (sentence) {
        FcitxLibPinyinUpdatePreedit(libpinyin, sentence);

        FcitxMessagesAddMessageAtLast(FcitxInputStateGetClientPreedit(input), MSG_INPUT, "%s", sentence);

        g_free(sentence);
    } else {
        FcitxInputStateSetCursorPos(input, libpinyin->cursor_pos);
        FcitxMessagesAddMessageAtLast(FcitxInputStateGetClientPreedit(input), MSG_INPUT, "%s", libpinyin->buf);
        FcitxMessagesAddMessageAtLast(FcitxInputStateGetPreedit(input), MSG_INPUT, "%s", libpinyin->buf);
    }

    pinyin_guess_candidates(libpinyin->inst, LibPinyinGetOffset(libpinyin));
    guint candidateLen = 0;
    pinyin_get_n_candidate(libpinyin->inst, &candidateLen);
    int i = 0;
    for (i = 0 ; i < candidateLen; i ++) {
        lookup_candidate_t* token = NULL;
        pinyin_get_candidate(libpinyin->inst, i, &token);
        FcitxCandidateWord candWord;
        FcitxLibPinyinCandWord* pyCand = (FcitxLibPinyinCandWord*) fcitx_utils_malloc0(sizeof(FcitxLibPinyinCandWord));
        pyCand->ispunc = false;
        pyCand->idx = i;
        candWord.callback = FcitxLibPinyinGetCandWord;
        candWord.extraType = MSG_OTHER;
        candWord.owner = libpinyin;
        candWord.priv = pyCand;
        candWord.strExtra = NULL;

        const gchar* phrase_string = NULL;
        pinyin_get_candidate_string(libpinyin->inst, token, &phrase_string);
        candWord.strWord = strdup(phrase_string);
        candWord.wordType = MSG_OTHER;

        FcitxCandidateWordAppend(FcitxInputStateGetCandidateList(input), &candWord);
    }

    return IRV_DISPLAY_CANDWORDS;
}

/**
 * @brief get the candidate word by index
 *
 * @param iIndex index of candidate word
 * @return the string of canidate word
 **/
INPUT_RETURN_VALUE FcitxLibPinyinGetCandWord(void* arg, FcitxCandidateWord* candWord)
{
    FcitxLibPinyin* libpinyin = (FcitxLibPinyin*)arg;
    FcitxLibPinyinCandWord* pyCand = (FcitxLibPinyinCandWord*) candWord->priv;
    FcitxInstance* instance = libpinyin->owner->owner;
    FcitxInputState* input = FcitxInstanceGetInputState(instance);

    if (pyCand->ispunc) {
        strcpy(FcitxInputStateGetOutputString(input), candWord->strWord);
        return IRV_COMMIT_STRING;
    } else {
        guint candidateLen = 0;
        pinyin_get_n_candidate(libpinyin->inst, &candidateLen);
        if (candidateLen <= pyCand->idx)
            return IRV_TO_PROCESS;
        lookup_candidate_t* cand = NULL;
        pinyin_get_candidate(libpinyin->inst, pyCand->idx, &cand);
        pinyin_choose_candidate(libpinyin->inst, LibPinyinGetOffset(libpinyin), cand);

        FcitxLibPinyinFixed f;
        const gchar* phrase_string = NULL;
        pinyin_get_candidate_string(libpinyin->inst, cand, &phrase_string);
        f.len = fcitx_utf8_strlen(phrase_string);
        g_array_append_val(libpinyin->fixed_string, f);

        int offset = LibPinyinGetOffset(libpinyin);

        guint pykeysLen = 0;
        pinyin_get_n_pinyin(libpinyin->inst, &pykeysLen);
        if (offset >= pykeysLen) {
            char* sentence = NULL;
            pinyin_guess_sentence(libpinyin->inst);
            sentence = LibPinyinGetSentence(libpinyin);
            if (sentence) {
                strcpy(FcitxInputStateGetOutputString(input), sentence);
                g_free(sentence);
                pinyin_train(libpinyin->inst);
            } else
                strcpy(FcitxInputStateGetOutputString(input), "");

            return IRV_COMMIT_STRING;
        }

        int pyoffset = LibPinyinGetPinyinOffset(libpinyin);
        if (pyoffset > libpinyin->cursor_pos)
            libpinyin->cursor_pos = pyoffset;

        return IRV_DISPLAY_CANDWORDS;
    }
    return IRV_TO_PROCESS;
}

FcitxLibPinyin* FcitxLibPinyinNew(FcitxLibPinyinAddonInstance* libpinyinaddon, LIBPINYIN_TYPE type)
{
    FcitxLibPinyin* libpinyin = (FcitxLibPinyin*) fcitx_utils_malloc0(sizeof(FcitxLibPinyin));
    libpinyin->inst = NULL;
    libpinyin->fixed_string = g_array_new(FALSE, FALSE, sizeof(FcitxLibPinyinFixed));
    libpinyin->type = type;
    libpinyin->owner = libpinyinaddon;
    return libpinyin;
}

void FcitxLibPinyinDelete(FcitxLibPinyin* libpinyin)
{
    if (libpinyin->inst)
        pinyin_free_instance(libpinyin->inst);
    g_array_free(libpinyin->fixed_string, TRUE);
}

void* LibPinyinSavePinyinWord(void* arg, FcitxModuleFunctionArg args)
{
    FcitxLibPinyinAddonInstance* libpinyinaddon = (FcitxLibPinyinAddonInstance*) arg;
    FcitxIM* im = FcitxInstanceGetCurrentIM(libpinyinaddon->owner);
    pinyin_context_t* context = NULL;
    if (strcmp(im->uniqueName, "pinyin-libpinyin") == 0 ||
            strcmp(im->uniqueName, "shuangpin-libpinyin") == 0) {
        context = libpinyinaddon->pinyin_context;
    }
    if (!context)
        return NULL;

    FcitxLibPinyin* libpinyin = (FcitxLibPinyin*) im->klass;

    std::stringstream ss;

    guint pinyinLen = 0;
    pinyin_get_n_pinyin(libpinyin->inst, &pinyinLen);
    for (int i = 0; i < pinyinLen; i ++) {
        PinyinKey* pykey;
        pinyin_get_pinyin_key(libpinyin->inst, i, &pykey);

        gchar* pystring;
        pinyin_get_pinyin_string(libpinyin->inst, pykey, &pystring);
        ss << pystring;
        g_free(pystring);
    }

    if (ss.str().length() > 0) {
        import_iterator_t* iter = pinyin_begin_add_phrases(context, 15);
        if (iter) {
            char* hz = (char*) args.args[0];
            pinyin_iterator_add_phrase(iter, hz, ss.str().c_str(), -1);
            pinyin_end_add_phrases(iter);
        }
    }
    pinyin_train(libpinyin->inst);

    return NULL;
}

/**
 * @brief initialize the extra input method
 *
 * @param arg
 * @return successful or not
 **/
void* FcitxLibPinyinCreate(FcitxInstance* instance)
{
    FcitxLibPinyinAddonInstance* libpinyinaddon = (FcitxLibPinyinAddonInstance*) fcitx_utils_malloc0(sizeof(FcitxLibPinyinAddonInstance));
    bindtextdomain("fcitx-libpinyin", LOCALEDIR);
    bind_textdomain_codeset("fcitx-libpinyin", "UTF-8");
    libpinyinaddon->owner = instance;
    FcitxAddon* addon = FcitxAddonsGetAddonByName(FcitxInstanceGetAddons(instance), "fcitx-libpinyin");

    if (!FcitxLibPinyinConfigLoadConfig(&libpinyinaddon->config)) {
        free(libpinyinaddon);
        return NULL;
    }

    libpinyinaddon->pinyin = FcitxLibPinyinNew(libpinyinaddon, LPT_Pinyin);
    libpinyinaddon->shuangpin = FcitxLibPinyinNew(libpinyinaddon, LPT_Shuangpin);
    libpinyinaddon->zhuyin = FcitxLibPinyinNew(libpinyinaddon, LPT_Zhuyin);

    FcitxLibPinyinReconfigure(libpinyinaddon);

    FcitxInstanceRegisterIM(instance,
                            libpinyinaddon->pinyin,
                            "pinyin-libpinyin",
                            _("Pinyin (LibPinyin)"),
                            "pinyin-libpinyin",
                            FcitxLibPinyinInit,
                            FcitxLibPinyinReset,
                            FcitxLibPinyinDoInput,
                            FcitxLibPinyinGetCandWords,
                            NULL,
                            FcitxLibPinyinSave,
                            NULL,
                            NULL,
                            5,
                            libpinyinaddon->config.bTraditionalDataForPinyin ? "zh_TW" : "zh_CN"
                           );

    FcitxInstanceRegisterIM(instance,
                            libpinyinaddon->shuangpin,
                            "shuangpin-libpinyin",
                            _("Shuangpin (LibPinyin)"),
                            "shuangpin-libpinyin",
                            FcitxLibPinyinInit,
                            FcitxLibPinyinReset,
                            FcitxLibPinyinDoInput,
                            FcitxLibPinyinGetCandWords,
                            NULL,
                            FcitxLibPinyinSave,
                            NULL,
                            NULL,
                            5,
                            libpinyinaddon->config.bTraditionalDataForPinyin ? "zh_TW" : "zh_CN"
                           );

    FcitxInstanceRegisterIM(instance,
                            libpinyinaddon->zhuyin,
                            "zhuyin-libpinyin",
                            _("Bopomofo (LibPinyin)"),
                            "bopomofo",
                            FcitxLibPinyinInit,
                            FcitxLibPinyinReset,
                            FcitxLibPinyinDoInput,
                            FcitxLibPinyinGetCandWords,
                            NULL,
                            FcitxLibPinyinSave,
                            NULL,
                            NULL,
                            5,
                            libpinyinaddon->config.bSimplifiedDataForZhuyin ? "zh_CN" : "zh_TW"
                           );

    FcitxModuleAddFunction(addon, LibPinyinSavePinyinWord);

    libpinyinaddon->bus = new FcitxLibPinyinBus(libpinyinaddon);

    return libpinyinaddon;
}

/**
 * @brief Destroy the input method while unload it.
 *
 * @return int
 **/
void FcitxLibPinyinDestroy(void* arg)
{
    FcitxLibPinyinAddonInstance* libpinyin = (FcitxLibPinyinAddonInstance*) arg;
    FcitxLibPinyinDelete(libpinyin->pinyin);
    FcitxLibPinyinDelete(libpinyin->zhuyin);
    FcitxLibPinyinDelete(libpinyin->shuangpin);
    if (libpinyin->pinyin_context)
        pinyin_fini(libpinyin->pinyin_context);
    if (libpinyin->zhuyin_context)
        pinyin_fini(libpinyin->zhuyin_context);

    delete libpinyin->bus;

    free(libpinyin);
}

void FcitxLibPinyinReconfigure(FcitxLibPinyinAddonInstance* libpinyinaddon)
{
    FcitxLibPinyinConfig* config = &libpinyinaddon->config;

    if (libpinyinaddon->zhuyin_context) {
        pinyin_set_chewing_scheme(libpinyinaddon->zhuyin_context, FcitxLibPinyinTransZhuyinLayout(config->zhuyinLayout));

        for (int i = 0; i <= FCITX_ZHUYIN_DICT_LAST; i++) {
            if (config->dict_zhuyin[i]) {
                pinyin_load_addon_phrase_library(libpinyinaddon->zhuyin_context, FcitxLibPinyinTransZhuyinDictionary(static_cast<FCITX_ZHUYIN_DICTIONARY>(i)));
            } else {
                pinyin_unload_addon_phrase_library(libpinyinaddon->zhuyin_context, FcitxLibPinyinTransZhuyinDictionary(static_cast<FCITX_ZHUYIN_DICTIONARY>(i)));
            }
        }
    }
    if (libpinyinaddon->pinyin_context) {
        pinyin_set_double_pinyin_scheme(libpinyinaddon->pinyin_context, FcitxLibPinyinTransShuangpinScheme(config->spScheme));
        for (int i = 0; i <= FCITX_DICT_LAST; i++) {
            if (config->dict[i]) {
                pinyin_load_addon_phrase_library(libpinyinaddon->pinyin_context, FcitxLibPinyinTransDictionary(static_cast<FCITX_DICTIONARY>(i)));
            } else {
                pinyin_unload_addon_phrase_library(libpinyinaddon->pinyin_context, FcitxLibPinyinTransDictionary(static_cast<FCITX_DICTIONARY>(i)));
            }
        }
    }
    pinyin_option_t settings = 0;
    settings |= DYNAMIC_ADJUST;
    settings |= USE_DIVIDED_TABLE | USE_RESPLIT_TABLE;
    for (int i = 0; i <= FCITX_CR_LAST; i ++) {
        if (config->cr[i])
            settings |= FcitxLibPinyinTransCorrection((FCITX_CORRECTION) i);
    }
    for (int i = 0; i <= FCITX_AMB_LAST; i ++) {
        if (config->amb[i])
            settings |= FcitxLibPinyinTransAmbiguity((FCITX_AMBIGUITY) i);
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

void FcitxLibPinyinReloadConfig(void* arg)
{
    FcitxLibPinyinAddonInstance* libpinyinaddon = (FcitxLibPinyinAddonInstance*) arg;
    FcitxLibPinyinConfigLoadConfig(&libpinyinaddon->config);
    FcitxLibPinyinReconfigure(libpinyinaddon);
}

void FcitxLibPinyinSave(void* arg)
{
    FcitxLibPinyin* libpinyin = (FcitxLibPinyin*) arg;
    if (libpinyin->owner->zhuyin_context)
        pinyin_save(libpinyin->owner->zhuyin_context);
    if (libpinyin->owner->pinyin_context)
        pinyin_save(libpinyin->owner->pinyin_context);
}

void FcitxLibPinyinClearData(FcitxLibPinyin* libpinyin, int type)
{
    FcitxLibPinyinAddonInstance* libpinyinaddon = libpinyin->owner;
    FcitxLibPinyinReset(libpinyin);

    pinyin_context_t* context = libpinyin->type != LPT_Zhuyin ? libpinyinaddon->pinyin_context : libpinyinaddon->zhuyin_context;
    if (!context) {
        return;
    }

    switch (type) {
    case 0:
        pinyin_mask_out(context, PHRASE_INDEX_LIBRARY_MASK, PHRASE_INDEX_MAKE_TOKEN(USER_DICTIONARY, null_token));
        pinyin_mask_out(context, PHRASE_INDEX_LIBRARY_MASK, PHRASE_INDEX_MAKE_TOKEN(ADDON_DICTIONARY, null_token));
        break;
    case 1:
        pinyin_mask_out(context, PHRASE_INDEX_LIBRARY_MASK, PHRASE_INDEX_MAKE_TOKEN(NETWORK_DICTIONARY, null_token));
        break;
    case 2:
        pinyin_mask_out(context, 0, 0);
        break;
    }

    pinyin_train(libpinyin->inst);
    pinyin_save(context);
}

void FcitxLibPinyinImport(FcitxLibPinyin* libpinyin)
{
    FcitxLibPinyinAddonInstance* libpinyinaddon = libpinyin->owner;
    FcitxLibPinyinReset(libpinyin);
    FcitxLibPinyinLoad(libpinyin);

    LIBPINYIN_LANGUAGE_TYPE langType = libpinyin->type == LPT_Zhuyin ?
                                       (libpinyinaddon->config.bSimplifiedDataForZhuyin ? LPLT_Simplified : LPLT_Traditional)
                                           : (libpinyinaddon->config.bTraditionalDataForPinyin ? LPLT_Traditional : LPLT_Simplified);

    pinyin_context_t* context = libpinyin->type != LPT_Zhuyin ? libpinyinaddon->pinyin_context : libpinyinaddon->zhuyin_context;
    if (!context)
        return;

    const char* path = langType == LPLT_Simplified ? "libpinyin/importdict" : "libpinyin/importdict_zhuyin";

    pinyin_mask_out(context, PHRASE_INDEX_LIBRARY_MASK, PHRASE_INDEX_MAKE_TOKEN(NETWORK_DICTIONARY, null_token));

    import_iterator_t* iter = pinyin_begin_add_phrases(context, NETWORK_DICTIONARY);
    if (iter == NULL) {
        return;
    }

    FcitxStringHashSet* sset = FcitxXDGGetFiles(path, NULL, ".txt");
    HASH_FOREACH(str, sset, FcitxStringHashSet) {
        /* user phrase library should be already loaded here. */
        FILE* dictfile = FcitxXDGGetFileWithPrefix(path, str->name, "r", NULL);
        if (NULL == dictfile) {
            continue;
        }

        char* linebuf = NULL;
        size_t size = 0;
        while (getline(&linebuf, &size, dictfile) != -1) {
            if (0 == strlen(linebuf))
                continue;

            if ('\n' == linebuf[strlen(linebuf) - 1]) {
                linebuf[strlen(linebuf) - 1] = '\0';
            }

            gchar** items = g_strsplit_set(linebuf, " \t", 3);
            guint len = g_strv_length(items);
            do {

                gchar* phrase = NULL, * pinyin = NULL;
                gint count = -1;
                if (2 == len || 3 == len) {
                    phrase = items[0];
                    pinyin = items[1];
                    if (3 == len)
                        count = atoi(items[2]);
                } else {
                    break;
                }

                if (!fcitx_utf8_check_string(phrase)) {
                    continue;
                }

                pinyin_iterator_add_phrase(iter, phrase, pinyin, count);
            } while (0);

            g_strfreev(items);
        }
        free(linebuf);
        fclose(dictfile);
    }

    pinyin_end_add_phrases(iter);

    if (libpinyin->inst) {
        pinyin_train(libpinyin->inst);
    }
    pinyin_save(context);
}

// kate: indent-mode cstyle; indent-width 4; replace-tabs on;
