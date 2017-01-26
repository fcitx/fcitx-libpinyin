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

#include <memory>
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

CONFIG_DEFINE_LOAD_AND_SAVE(FcitxLibPinyinConfig, FcitxLibPinyinConfig, "fcitx-libpinyin")

static void FcitxLibPinyinReconfigure(FcitxLibPinyinAddonInstance* libpinyin);
static void FcitxLibPinyinSave(void* arg);
static bool LibPinyinCheckZhuyinKey(FcitxKeySym sym, FCITX_ZHUYIN_LAYOUT layout, boolean useTone) ;

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

int FcitxLibPinyin::offset() const
{
    if (!m_fixedString.empty()) {
        return m_fixedString.back().first;
    }
    return 0;
}

int FcitxLibPinyin::pinyinOffset() const
{
    if (!m_fixedString.empty()) {
        return m_fixedString.back().second;
    }
    return 0;
}

/**
 * @brief Reset the status.
 *
 **/
void FcitxLibPinyinReset(void* arg)
{
    FcitxLibPinyin* libpinyin = (FcitxLibPinyin*) arg;
    libpinyin->reset();
}

void FcitxLibPinyin::reset() {
    m_buf.clear();
    m_cursorPos = 0;
    m_parsedLen = 0;
    m_fixedString.clear();
    if (m_inst) {
        pinyin_reset(m_inst);
    }
}

size_t FcitxLibPinyin::parse(const char* str)
{
    switch (m_type) {
    case LPT_Pinyin:
        return pinyin_parse_more_full_pinyins(m_inst, str);
    case LPT_Shuangpin:
        return pinyin_parse_more_double_pinyins(m_inst, str);
    case LPT_Zhuyin:
        return pinyin_parse_more_chewings(m_inst, str);
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
    return libpinyin->doInput(sym, state);
}

INPUT_RETURN_VALUE FcitxLibPinyin::doInput(FcitxKeySym sym, unsigned int state) {
    FcitxLibPinyinConfig* config = &m_owner->config;
    FcitxInstance* instance = m_owner->owner;
    FcitxInputState* input = FcitxInstanceGetInputState(instance);

    if (FcitxHotkeyIsHotKeySimple(sym, state)) {
        /* there is some special case that ';' is used */
        if (FcitxHotkeyIsHotKeyLAZ(sym, state)
                || sym == '\''
                || (FcitxHotkeyIsHotKey(sym, state, FCITX_SEMICOLON) && m_type == LPT_Shuangpin && (config->spScheme == FCITX_SHUANG_PIN_MS || config->spScheme == FCITX_SHUANG_PIN_ZIGUANG))
                || (m_type == LPT_Zhuyin && LibPinyinCheckZhuyinKey(sym, config->zhuyinLayout, config->useTone))
           ) {
            if (m_buf.size() == 0 && (sym == '\'' || sym == ';'))
                return IRV_TO_PROCESS;

            if (m_buf.size() < MAX_PINYIN_INPUT) {
                m_buf.insert(m_cursorPos, 1, (char)(sym & 0xff));
                m_cursorPos ++;

                m_parsedLen =parse(m_buf.c_str());

                if (pinyin_get_parsed_input_length(m_inst) == 0 && m_buf.size() == 1 && m_type != LPT_Shuangpin
                        && !(m_type == LPT_Pinyin && !m_owner->config.incomplete)
                        && !(m_type == LPT_Zhuyin && !m_owner->config.zhuyinIncomplete)) {
                    reset();
                    return IRV_TO_PROCESS;
                }
                return IRV_DISPLAY_CANDWORDS;
            } else
                return IRV_DO_NOTHING;
        }
    }

    if (FcitxHotkeyIsHotKey(sym, state, FCITX_SPACE)
            || (m_type == LPT_Zhuyin && FcitxHotkeyIsHotKey(sym, state, FCITX_ENTER))) {
        size_t len = m_buf.size();
        if (len == 0)
            return IRV_TO_PROCESS;

        return FcitxCandidateWordChooseByIndex(FcitxInputStateGetCandidateList(input), 0);
    }

    if ((m_type != LPT_Zhuyin && FcitxHotkeyIsHotKey(sym, state, FCITX_ENTER))
        || (m_type == LPT_Zhuyin && FcitxHotkeyIsHotKey(sym, state, FCITX_LIBPINYIN_SHIFT_ENTER))) {
        if (m_buf[0] == 0)
            return IRV_TO_PROCESS;

        const std::string sentence = this->sentence();
        if (!sentence.empty()) {
            int offset = this->offset();
            int hzlen = 0;
            if (fcitx_utf8_strlen(sentence.c_str()) > offset) {
                hzlen = fcitx_utf8_get_nth_char(sentence.c_str(), offset) - sentence.c_str();
            } else {
                hzlen = sentence.size();
            }

            int start = pinyinOffset();
            int pylen = m_buf.size() - start;

            if (pylen < 0) {
                pylen = 0;
            }

            char* buf = (char*) fcitx_utils_malloc0((hzlen + pylen + 1) * sizeof(char));
            strncpy(buf, sentence.c_str(), hzlen);
            if (pylen) {
                strcpy(&buf[hzlen], &m_buf[start]);
            }
            buf[hzlen + pylen] = '\0';
            FcitxInstanceCommitString(instance, FcitxInstanceGetCurrentIC(instance), buf);
            free(buf);
        } else {
            FcitxInstanceCommitString(instance, FcitxInstanceGetCurrentIC(instance), m_buf.c_str());
        }

        return IRV_CLEAN;
    }

    if (FcitxHotkeyIsHotKey(sym, state, FCITX_BACKSPACE) || FcitxHotkeyIsHotKey(sym, state, FCITX_DELETE)) {
        if (m_buf.size() > 0) {
            if (offset() != 0 && FcitxHotkeyIsHotKey(sym, state, FCITX_BACKSPACE)) {
                m_fixedString.pop_back();
                pinyin_clear_constraint(m_inst, pinyinOffset());
            } else {
                if (FcitxHotkeyIsHotKey(sym, state, FCITX_BACKSPACE)) {
                    if (m_cursorPos > 0) {
                        m_cursorPos -- ;
                    } else {
                        return IRV_DO_NOTHING;
                    }
                }
                size_t len = m_buf.size();
                if (m_cursorPos == (int)len)
                    return IRV_DO_NOTHING;
                m_buf.erase(m_cursorPos, 1);
                if (m_buf.empty())
                    return IRV_CLEAN;
                else
                    m_parsedLen = parse(m_buf.c_str());
            }
            return IRV_DISPLAY_CANDWORDS;
        } else
            return IRV_TO_PROCESS;
    } else {
        if (m_buf.size() > 0) {
            if (FcitxHotkeyIsHotKey(sym, state, FCITX_LEFT)) {
                if (m_cursorPos > 0) {
                    if (m_cursorPos == pinyinOffset()) {
                        m_fixedString.pop_back();
                        pinyin_clear_constraint(m_inst, offset());
                        return IRV_DISPLAY_CANDWORDS;
                    } else {
                        m_cursorPos--;
                        return IRV_DISPLAY_CANDWORDS;
                    }
                }

                return IRV_DO_NOTHING;
            } else if (FcitxHotkeyIsHotKey(sym, state, FCITX_RIGHT)) {
                size_t len = m_buf.size();
                if (m_cursorPos < (int) len) {
                    m_cursorPos ++ ;
                    return IRV_DISPLAY_CANDWORDS;
                }
                return IRV_DO_NOTHING;
            } else if (FcitxHotkeyIsHotKey(sym, state, FCITX_HOME)) {
                int offset = pinyinOffset();
                if (m_cursorPos != offset) {
                    m_cursorPos = offset;
                    return IRV_DISPLAY_CANDWORDS;
                }
                return IRV_DO_NOTHING;
            } else if (FcitxHotkeyIsHotKey(sym, state, FCITX_END)) {
                size_t len = m_buf.size();
                if (m_cursorPos != (int) len) {
                    m_cursorPos = len ;
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

void FcitxLibPinyin::load()
{
    if (m_inst != NULL)
        return;

    FcitxLibPinyinAddonInstance* libpinyinaddon = m_owner;

    if (m_type == LPT_Zhuyin && m_owner->zhuyin_context == NULL) {
        char* user_path = FcitxLibPinyinGetUserPath(libpinyinaddon->config.bSimplifiedDataForZhuyin ? LPLT_Simplified : LPLT_Traditional);
        char* syspath = FcitxLibPinyinGetSysPath(libpinyinaddon->config.bSimplifiedDataForZhuyin ? LPLT_Simplified : LPLT_Traditional);
        libpinyinaddon->zhuyin_context = pinyin_init(syspath, user_path);
        free(user_path);
        free(syspath);
    }

    if (m_type != LPT_Zhuyin && m_owner->pinyin_context == NULL) {
        char* user_path = FcitxLibPinyinGetUserPath(libpinyinaddon->config.bTraditionalDataForPinyin ? LPLT_Traditional : LPLT_Simplified);
        char* syspath = FcitxLibPinyinGetSysPath(libpinyinaddon->config.bTraditionalDataForPinyin ? LPLT_Traditional : LPLT_Simplified);
        libpinyinaddon->pinyin_context = pinyin_init(syspath, user_path);
        free(user_path);
        free(syspath);
    }

    if (m_type == LPT_Zhuyin)
        m_inst = pinyin_alloc_instance(libpinyinaddon->zhuyin_context);
    else
        m_inst = pinyin_alloc_instance(libpinyinaddon->pinyin_context);

    FcitxLibPinyinReconfigure(libpinyinaddon);
}

boolean FcitxLibPinyinInit(void* arg)
{
    FcitxLibPinyin* libpinyin = (FcitxLibPinyin*) arg;
    libpinyin->init();
    return true;
}

void FcitxLibPinyin::init() {
    FcitxInstanceSetContext(m_owner->owner, CONTEXT_IM_KEYBOARD_LAYOUT, "us");
    if (m_type == LPT_Zhuyin) {
        FcitxInstanceSetContext(m_owner->owner, CONTEXT_ALTERNATIVE_PREVPAGE_KEY, m_owner->config.hkPrevPage);
        FcitxInstanceSetContext(m_owner->owner, CONTEXT_ALTERNATIVE_NEXTPAGE_KEY, m_owner->config.hkNextPage);
    }

    load();
}

void FcitxLibPinyin::updatePreedit(const std::string &sentence)
{
    FcitxInstance* instance = m_owner->owner;
    FcitxInputState* input = FcitxInstanceGetInputState(instance);
    int offset = this->offset();

#if 0
    if (m_type == LPT_Pinyin) {
        const gchar* raw_full_pinyin;
        pinyin_get_raw_full_pinyin(m_inst, &raw_full_pinyin);
        int libpinyinLen = strlen(raw_full_pinyin);
        int fcitxLen = m_buf.size();
        if (fcitxLen != libpinyinLen) {
            strcpy(m_buf, raw_full_pinyin);
            m_cursorPos += libpinyinLen - fcitxLen;
        }
    }
#endif

    int pyoffset = pinyinOffset();
    if (pyoffset > m_cursorPos)
        m_cursorPos = pyoffset;

    int hzlen = 0;
    if (fcitx_utf8_strlen(sentence.c_str()) > offset) {
        hzlen = fcitx_utf8_get_nth_char(sentence.c_str(), offset) - sentence.c_str();
    } else {
        hzlen = sentence.size();
    }

    if (hzlen > 0) {
        char* buf = (char*) fcitx_utils_malloc0((hzlen + 1) * sizeof(char));
        strncpy(buf, sentence.c_str(), hzlen);
        buf[hzlen] = 0;
        FcitxMessagesAddMessageAtLast(FcitxInputStateGetPreedit(input), MSG_INPUT, "%s", buf);
        free(buf);
    }

    int charcurpos = hzlen;

    int lastpos = pyoffset;
    int curoffset = pyoffset;

    PinyinKey* pykey = NULL;
    PinyinKeyPos* pykeypos = NULL;
    FcitxMessagesAddMessageAtLast(FcitxInputStateGetPreedit(input), MSG_CODE, "");
    for (int i = pinyinOffset(); i < m_parsedLen; ) {
        if (!pinyin_get_pinyin_key(m_inst, i, &pykey)) {
            break;
        }
        pinyin_get_pinyin_key_rest(m_inst, i, &pykeypos);

        guint16 rawBegin = 0, rawEnd = 0;
        pinyin_get_pinyin_key_rest_positions(m_inst, pykeypos, &rawBegin, &rawEnd);
        if (lastpos > 0) {
            FcitxMessagesMessageConcatLast(FcitxInputStateGetPreedit(input), " ");
            if (curoffset < m_cursorPos)
                charcurpos ++;
            for (int j = lastpos; j < rawBegin; j ++) {
                char temp[2] = {'\0', '\0'};
                temp[0] = m_buf[j];
                FcitxMessagesMessageConcatLast(FcitxInputStateGetPreedit(input), temp);
                if (curoffset < m_cursorPos) {
                    curoffset ++;
                    charcurpos ++;
                }
            }
            if (lastpos < rawBegin) {
                FcitxMessagesMessageConcatLast(FcitxInputStateGetPreedit(input), " ");
                if (curoffset < m_cursorPos)
                    charcurpos ++;
            }
        }
        lastpos = rawEnd;

        bool breakAhead = false;
        switch (m_type) {
        case LPT_Pinyin: {
            gchar* pystring;
            pinyin_get_pinyin_string(m_inst, pykey, &pystring);
            if (!pystring) {
                breakAhead = true;
                break;
            }
            FcitxMessagesMessageConcatLast(FcitxInputStateGetPreedit(input), pystring);
            size_t pylen = strlen(pystring);
            if (curoffset + pylen < m_cursorPos) {
                curoffset += pylen;
                charcurpos += pylen;
            } else {
                charcurpos += m_cursorPos - curoffset;
                curoffset = m_cursorPos;
            }
            g_free(pystring);
            break;
        }
        case LPT_Shuangpin: {
            guint16 pykeyposLen = 0;
            pinyin_get_pinyin_key_rest_length(m_inst, pykeypos, &pykeyposLen);
            if (pykeyposLen == 2) {
                const gchar* initial = NULL;
                gchar* middle_final = NULL, *_initial = NULL;
                pinyin_get_pinyin_strings(m_inst, pykey, &_initial, &middle_final);
                if (!_initial[0])
                    initial = "'";
                else
                    initial = _initial;
                if (curoffset + 1 <= m_cursorPos) {
                    curoffset += 1;
                    charcurpos += strlen(initial);
                }
                FcitxMessagesMessageConcatLast(FcitxInputStateGetPreedit(input), initial);

                if (curoffset + 1 <= m_cursorPos) {
                    curoffset += 1;
                    charcurpos += strlen(middle_final);
                }
                FcitxMessagesMessageConcatLast(FcitxInputStateGetPreedit(input), middle_final);
                g_free(_initial);
                g_free(middle_final);
            } else if (pykeyposLen == 1) {
                gchar* pystring;
                pinyin_get_pinyin_string(m_inst, pykey, &pystring);
                if (curoffset + 1 <= m_cursorPos) {
                    curoffset += 1;
                    charcurpos += strlen(pystring);
                }
                FcitxMessagesMessageConcatLast(FcitxInputStateGetPreedit(input), pystring);
                g_free(pystring);
            }
            break;
        }
        case LPT_Zhuyin: {
            guint16 pykeyposLen = 0;
            pinyin_get_pinyin_key_rest_length(m_inst, pykeypos, &pykeyposLen);
            gchar* pystring;
            pinyin_get_zhuyin_string(m_inst, pykey, &pystring);
            // libpinyin would give us null when it is something like "xi'"
            if (!pystring) {
                breakAhead = true;
                break;
            }
            FcitxMessagesMessageConcatLast(FcitxInputStateGetPreedit(input), pystring);

            if (curoffset + pykeyposLen <= m_cursorPos) {
                curoffset += pykeyposLen;
                charcurpos += strlen(pystring);
            } else {
                int diff = m_cursorPos - curoffset;
                curoffset = m_cursorPos;
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
        if (breakAhead) {
            break;
        }
        // assertion would happen if pinyin string is sth like "xi'"
        // so we use breakAhead to track such case. It's ugly but also works.
        size_t nexti;
        if (pinyin_get_right_pinyin_offset(m_inst, i, &nexti)) {
            i = nexti;
        } else {
            break;
        }
    }

    int buflen = m_buf.size();

    if (lastpos < buflen) {
        if (FcitxMessagesGetMessageCount(FcitxInputStateGetPreedit(input))) {
            FcitxMessagesMessageConcatLast(FcitxInputStateGetPreedit(input), " ");
            if (lastpos < m_cursorPos)
                charcurpos ++;
        }

        for (int i = lastpos; i < buflen; i ++) {
            char temp[2] = {'\0', '\0'};
            temp[0] = m_buf[i];
            FcitxMessagesMessageConcatLast(FcitxInputStateGetPreedit(input), temp);
            if (lastpos < m_cursorPos) {
                charcurpos ++;
                lastpos++;
            }
        }
    }
    FcitxInputStateSetCursorPos(input, charcurpos);
}

std::string FcitxLibPinyin::sentence()
{
    char* sentence = NULL;
    pinyin_get_sentence(m_inst, &sentence);
    std::string result = sentence ? sentence : "";
    g_free(sentence);
    return result;
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
    return libpinyin->getCandWords();
}

INPUT_RETURN_VALUE FcitxLibPinyin::getCandWords() {
    FcitxInstance* instance = m_owner->owner;
    FcitxInputState* input = FcitxInstanceGetInputState(instance);
    FcitxGlobalConfig* config = FcitxInstanceGetGlobalConfig(m_owner->owner);
    FcitxLibPinyinConfig* pyConfig = &m_owner->config;
    struct _FcitxCandidateWordList* candList = FcitxInputStateGetCandidateList(input);
    FcitxCandidateWordSetPageSize(candList, config->iMaxCandWord);
    FcitxUICloseInputWindow(instance);
    strcpy(FcitxInputStateGetRawInputBuffer(input), m_buf.c_str());
    FcitxInputStateSetRawInputBufferSize(input, m_buf.size());
    FcitxInputStateSetShowCursor(input, true);
    FcitxInputStateSetClientCursorPos(input, 0);

    if (m_type == LPT_Zhuyin) {
        FcitxKeyState state = candidateModifierMap[pyConfig->candidateModifiers];
        FcitxCandidateWordSetChooseAndModifier(candList, "1234567890", state);
    } else
        FcitxCandidateWordSetChoose(candList, "1234567890");

    /* add punc */
    if (m_type == LPT_Zhuyin
            && m_buf.size() == 1
            && LibPinyinCheckZhuyinKey((FcitxKeySym) m_buf[0], pyConfig->zhuyinLayout, pyConfig->useTone)
            && (m_buf[0] >= ' ' && m_buf[0] <= '\x7e') /* simple */
            && !(m_buf[0] >= 'a' && m_buf[0] <= 'z') /* not a-z */
            && !(m_buf[0] >= 'A' && m_buf[0] <= 'Z') /* not A-Z /*/
            && !(m_buf[0] >= '0' && m_buf[0] <= '9') /* not digit */
       ) {
        int c = m_buf[0];
        char* result = FcitxPuncGetPunc(instance, &c);
        if (result) {
            FcitxCandidateWord candWord;
            FcitxLibPinyinCandWord* pyCand = (FcitxLibPinyinCandWord*) fcitx_utils_malloc0(sizeof(FcitxLibPinyinCandWord));
            pyCand->ispunc = true;
            candWord.callback = FcitxLibPinyinGetCandWord;
            candWord.extraType = MSG_OTHER;
            candWord.owner = this;
            candWord.priv = pyCand;
            candWord.strExtra = NULL;
            candWord.strWord = strdup(result);
            candWord.wordType = MSG_OTHER;

            FcitxCandidateWordAppend(FcitxInputStateGetCandidateList(input), &candWord);
        }
    }
    pinyin_guess_sentence(m_inst);
    const std::string sentence = this->sentence();
    if (!sentence.empty()) {
        updatePreedit(sentence.c_str());

        FcitxMessagesAddMessageAtLast(FcitxInputStateGetClientPreedit(input), MSG_INPUT, "%s", sentence.c_str());
        if (m_buf.size() >= m_parsedLen) {
            FcitxMessagesAddMessageAtLast(FcitxInputStateGetClientPreedit(input), MSG_INPUT, "%s", m_buf.substr(m_parsedLen).c_str());
        }
    } else {
        FcitxInputStateSetCursorPos(input, m_cursorPos);
        FcitxMessagesAddMessageAtLast(FcitxInputStateGetClientPreedit(input), MSG_INPUT, "%s", m_buf.c_str());
        FcitxMessagesAddMessageAtLast(FcitxInputStateGetPreedit(input), MSG_INPUT, "%s", m_buf.c_str());
    }

    if (pinyinOffset() < m_parsedLen) {
        pinyin_guess_candidates(m_inst, pinyinOffset());
        guint candidateLen = 0;
        pinyin_get_n_candidate(m_inst, &candidateLen);
        int i = 0;
        for (i = 0 ; i < candidateLen; i ++) {
            lookup_candidate_t* token = NULL;
            pinyin_get_candidate(m_inst, i, &token);
            FcitxCandidateWord candWord;
            FcitxLibPinyinCandWord* pyCand = (FcitxLibPinyinCandWord*) fcitx_utils_malloc0(sizeof(FcitxLibPinyinCandWord));
            pyCand->ispunc = false;
            pyCand->idx = i;
            candWord.callback = FcitxLibPinyinGetCandWord;
            candWord.extraType = MSG_OTHER;
            candWord.owner = this;
            candWord.priv = pyCand;
            candWord.strExtra = NULL;

            const gchar* phrase_string = NULL;
            pinyin_get_candidate_string(m_inst, token, &phrase_string);
            candWord.strWord = strdup(phrase_string);
            candWord.wordType = MSG_OTHER;

            FcitxCandidateWordAppend(FcitxInputStateGetCandidateList(input), &candWord);
        }
    } else {
        FcitxCandidateWord candWord;
        FcitxLibPinyinCandWord* pyCand = (FcitxLibPinyinCandWord*) fcitx_utils_malloc0(sizeof(FcitxLibPinyinCandWord));
        pyCand->ispunc = false;
        pyCand->idx = -1;
        candWord.callback = FcitxLibPinyinGetCandWord;
        candWord.extraType = MSG_OTHER;
        candWord.owner = this;
        candWord.priv = pyCand;
        candWord.strExtra = NULL;
        std::string cand;
        if (m_buf.size() >= m_parsedLen) {
            cand += m_buf.substr(m_parsedLen);
        }

        candWord.strWord = strdup(cand.c_str());
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
    return libpinyin->getCandWord(candWord);
}

INPUT_RETURN_VALUE FcitxLibPinyin::getCandWord(FcitxCandidateWord* candWord) {
    FcitxLibPinyinCandWord* pyCand = (FcitxLibPinyinCandWord*) candWord->priv;
    FcitxInstance* instance = m_owner->owner;
    FcitxInputState* input = FcitxInstanceGetInputState(instance);

    if (pyCand->ispunc) {
        strcpy(FcitxInputStateGetOutputString(input), candWord->strWord);
        return IRV_COMMIT_STRING;
    } else if (pyCand->idx < 0) {
        strcpy(FcitxInputStateGetOutputString(input), (sentence() + candWord->strWord).c_str());
        return IRV_COMMIT_STRING;
    } else {
        guint candidateLen = 0;
        pinyin_get_n_candidate(m_inst, &candidateLen);
        if (candidateLen <= pyCand->idx)
            return IRV_TO_PROCESS;
        lookup_candidate_t* cand = NULL;
        pinyin_get_candidate(m_inst, pyCand->idx, &cand);
        int newOffset = pinyin_choose_candidate(m_inst, pinyinOffset(), cand);
        if (newOffset != pinyinOffset()) {
            const gchar* phrase_string = NULL;
            pinyin_get_candidate_string(m_inst, cand, &phrase_string);
            m_fixedString.push_back(std::make_pair(offset() + fcitx_utf8_strlen(phrase_string), newOffset));
        }

        if (pinyinOffset() == m_parsedLen) {
            if (m_parsedLen == m_buf.size()) {
                pinyin_guess_sentence(m_inst);
                const std::string sentence = this->sentence();
                if (!sentence.empty()) {
                    strcpy(FcitxInputStateGetOutputString(input), sentence.c_str());
                    pinyin_train(m_inst);
                } else
                    strcpy(FcitxInputStateGetOutputString(input), "");

                return IRV_COMMIT_STRING;
            }
        }

        int pyoffset = pinyinOffset();
        if (pyoffset > m_cursorPos)
            m_cursorPos = pyoffset;

        return IRV_DISPLAY_CANDWORDS;
    }
    return IRV_TO_PROCESS;
}

FcitxLibPinyin::FcitxLibPinyin(FcitxLibPinyinAddonInstance* libpinyinaddon, LIBPINYIN_TYPE type) :
    m_inst(NULL), m_type(type), m_owner(libpinyinaddon)
{
}

FcitxLibPinyin::~FcitxLibPinyin()
{
    if (m_inst)
        pinyin_free_instance(m_inst);
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
    libpinyin->savePinyinWord(static_cast<const char*>(args.args[0]));

    return NULL;
}

void FcitxLibPinyin::savePinyinWord(const char* str) {
    // valid non empty utf8 string
    if (fcitx_utf8_check_string(str) && *str) {
        const char *s = str;
        while (*s) {
            uint32_t chr;

            s = fcitx_utf8_get_char(s, &chr);
            // ok ok, let's filter out ascii
            if (chr < 256) {
                return;
            }
        }
        pinyin_remember_user_input(m_inst, str, -1);
    }
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

    libpinyinaddon->pinyin = new FcitxLibPinyin(libpinyinaddon, LPT_Pinyin);
    libpinyinaddon->shuangpin = new FcitxLibPinyin(libpinyinaddon, LPT_Shuangpin);
    libpinyinaddon->zhuyin = new FcitxLibPinyin(libpinyinaddon, LPT_Zhuyin);

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
    delete libpinyin->pinyin;
    delete libpinyin->zhuyin;
    delete libpinyin->shuangpin;

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
        pinyin_set_zhuyin_scheme(libpinyinaddon->zhuyin_context, FcitxLibPinyinTransZhuyinLayout(config->zhuyinLayout));

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

    if (config->zhuyinIncomplete) {
        settings |= ZHUYIN_INCOMPLETE;
    }

    if (config->useTone) {
        settings |= USE_TONE;
    }
    settings |= IS_PINYIN;
    settings |= IS_ZHUYIN;
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
    libpinyin->save();
}

void FcitxLibPinyin::save() {
    if (m_owner->zhuyin_context)
        pinyin_save(m_owner->zhuyin_context);
    if (m_owner->pinyin_context)
        pinyin_save(m_owner->pinyin_context);
}

void FcitxLibPinyin::clearData(int type)
{
    FcitxLibPinyinAddonInstance* libpinyinaddon = m_owner;
    reset();

    pinyin_context_t* context = m_type != LPT_Zhuyin ? libpinyinaddon->pinyin_context : libpinyinaddon->zhuyin_context;
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

    pinyin_train(m_inst);
    pinyin_save(context);
}

void FcitxLibPinyin::import()
{
    FcitxLibPinyinAddonInstance* libpinyinaddon = m_owner;
    reset();
    load();

    LIBPINYIN_LANGUAGE_TYPE langType = m_type == LPT_Zhuyin ?
                                       (libpinyinaddon->config.bSimplifiedDataForZhuyin ? LPLT_Simplified : LPLT_Traditional)
                                           : (libpinyinaddon->config.bTraditionalDataForPinyin ? LPLT_Traditional : LPLT_Simplified);

    pinyin_context_t* context = m_type != LPT_Zhuyin ? libpinyinaddon->pinyin_context : libpinyinaddon->zhuyin_context;
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

    if (m_inst) {
        pinyin_train(m_inst);
    }
    pinyin_save(context);
}

// kate: indent-mode cstyle; indent-width 4; replace-tabs on;
