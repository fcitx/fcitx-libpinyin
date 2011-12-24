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
#include <string>
#include <libintl.h>

#include "config.h"
#include "eim.h"
#include "enummap.h"


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
    phrase_token_t token;
} FcitxLibpinyinCandWord;

typedef struct _FcitxLibpinyinFixed {
    int len;
    phrase_token_t token;
} FcitxLibpinyinFixed;

CONFIG_DESC_DEFINE(GetLibpinyinConfigDesc, "fcitx-libpinyin.desc")

boolean LoadLibpinyinConfig(FcitxLibpinyinConfig* fs);
static void SaveLibpinyinConfig(FcitxLibpinyinConfig* fs);
static void ConfigLibpinyin(FcitxLibpinyinAddonInstance* libpinyin);
static int LibpinyinGetOffset(FcitxLibpinyin* libpinyin);
static void FcitxLibpinyinSave(void *arg);
static bool LibpinyinCheckZhuyinKey(FcitxKeySym sym, FCITX_ZHUYIN_LAYOUT layout) ;

static const char *input_keys [] = {
     "1qaz2wsxedcrfv5tgbyhnujm8ik,9ol.0p;/-7634",       /* standard kb */
     "bpmfdtnlgkhjvcjvcrzasexuyhgeiawomnkllsdfj",       /* hsu */
     "1234567890-qwertyuiopasdfghjkl;zxcvbn/m,.",       /* IBM */
     "2wsx3edcrfvtgb6yhnujm8ik,9ol.0p;/-['=1qaz",       /* Gin-yieh */
     "bpmfdtnlvkhg7c,./j;'sexuaorwiqzy890-=1234",       /* ET  */
     "bpmfdtnlvkhgvcgycjqwsexuaorwiqzpmntlhdfjk",       /* ET26 */
     0
};

bool LibpinyinCheckZhuyinKey(FcitxKeySym sym, FCITX_ZHUYIN_LAYOUT layout) {
    char key = sym & 0xff;
    const char* keys = input_keys[layout];
    if (strchr(keys, key))
        return true;
    else
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
    int i = FCITX_LIBPINYIN_MIN(offset, libpinyin->inst->m_pinyin_poses->len) - 1;
    if (i >= 0)
    {
        PinyinKeyPos* pykey = &g_array_index(libpinyin->inst->m_pinyin_poses, PinyinKeyPos, i);
        pyoffset = pykey->get_end_pos();
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
            || (libpinyin->type == LPT_Zhuyin && LibpinyinCheckZhuyinKey(sym, config->zhuyinLayout))
        )
        {
            if (strlen(libpinyin->buf) == 0 && sym == ('\'' || sym == ';'))
                return IRV_TO_PROCESS;
            
            if (strlen(libpinyin->buf) < MAX_USER_INPUT)
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
                
                if (parselen == 0 && strlen(libpinyin->buf) == 1 && libpinyin->type != LPT_Shuangpin)
                {
                    FcitxLibpinyinReset(libpinyin);
                    return IRV_TO_PROCESS;
                }
                return IRV_DISPLAY_CANDWORDS;
            }
            else
                return IRV_DO_NOTHING;
        }
        else if (FcitxHotkeyIsHotKey(sym, state, FCITX_SPACE))
        {
            size_t len = strlen(libpinyin->buf);
            if (len == 0)
                return IRV_TO_PROCESS;

            return FcitxCandidateWordChooseByIndex(FcitxInputStateGetCandidateList(input), 0);
        }
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

boolean FcitxLibpinyinInit(void* arg)
{
    return true;
}

void FcitxLibpinyinUpdatePreedit(FcitxLibpinyin* libpinyin, char* sentence)
{
    FcitxInstance* instance = libpinyin->owner->owner;
    FcitxInputState* input = FcitxInstanceGetInputState(instance);
    int offset = LibpinyinGetOffset(libpinyin);
    
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
        PinyinKeyPos* pykeypos = &g_array_index(libpinyin->inst->m_pinyin_poses, PinyinKeyPos, i);
        
        if (lastpos > 0) {
            FcitxMessagesMessageConcatLast (FcitxInputStateGetPreedit(input), " ");
            if (curoffset < libpinyin->cursor_pos)
                charcurpos ++;
            for (int j = lastpos; j < pykeypos->get_pos(); j ++) {
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
        lastpos = pykeypos->get_end_pos();
        
        switch (libpinyin->type) {
            case LPT_Pinyin: {
                FcitxMessagesAddMessageAtLast(FcitxInputStateGetPreedit(input), MSG_CODE, "%s", pykey->get_key_string());
                size_t pylen = strlen(pykey->get_key_string());
                if (curoffset + pylen < libpinyin->cursor_pos) {
                    curoffset += pylen;
                    charcurpos += pylen;
                }
                else {
                    charcurpos += libpinyin->cursor_pos - curoffset;
                    curoffset = libpinyin->cursor_pos;
                }
                break;
            }
            case LPT_Shuangpin: {
                if (pykeypos->get_length() == 2) {
                    const char* initial = 0;
                    if (strlen(pykey->get_initial_string()) == 0)
                        initial = "*";
                    else
                        initial = pykey->get_initial_string();
                    if (curoffset + 1 <= libpinyin->cursor_pos) {
                        curoffset += 1;
                        charcurpos += strlen(initial);
                    }
                    FcitxMessagesAddMessageAtLast(FcitxInputStateGetPreedit(input), MSG_CODE, "%s", initial);
                    
                    if (curoffset + 1 <= libpinyin->cursor_pos) {
                        curoffset += 1;
                        charcurpos += strlen(pykey->get_final_string());
                    }
                    FcitxMessagesAddMessageAtLast(FcitxInputStateGetPreedit(input), MSG_CODE, "%s", pykey->get_final_string());
                }
                else if (pykeypos->get_length() == 1) {
                    if (curoffset + 1 <= libpinyin->cursor_pos) {
                        curoffset += 1;
                        charcurpos += strlen(pykey->get_key_string());
                    }
                    FcitxMessagesAddMessageAtLast(FcitxInputStateGetPreedit(input), MSG_CODE, "%s", pykey->get_key_string());
                }
                break;
            }
            case LPT_Zhuyin: {
                const char* final = 0;
                if (curoffset + 1 <= libpinyin->cursor_pos) {
                    curoffset += 1;
                    charcurpos += strlen(pykey->get_initial_zhuyin_string());
                }
                FcitxMessagesAddMessageAtLast(FcitxInputStateGetPreedit(input), MSG_CODE, "%s", pykey->get_initial_zhuyin_string());
                
                if (!(pykeypos->get_length() == 1 || (pykeypos->get_length() == 2 && pykey->get_tone() != PINYIN_ZeroTone))) {
                    if (strlen(pykey->get_final_zhuyin_string()) == 0)
                        final = ".";
                    else
                        final = pykey->get_final_zhuyin_string();
                    if (curoffset + 1 <= libpinyin->cursor_pos) {
                        curoffset += 1;
                        charcurpos += strlen(final);
                    }
                    FcitxMessagesAddMessageAtLast(FcitxInputStateGetPreedit(input), MSG_CODE, "%s", final);
                }

                if (strlen(pykey->get_tone_zhuyin_string()) != 0)
                {
                    if (curoffset + 1 <= libpinyin->cursor_pos) {
                        curoffset += 1;
                        charcurpos += strlen(pykey->get_tone_zhuyin_string());
                    }
                    FcitxMessagesAddMessageAtLast(FcitxInputStateGetPreedit(input), MSG_CODE, "%s", pykey->get_tone_zhuyin_string());
                }
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
    struct _FcitxCandidateWordList* candList = FcitxInputStateGetCandidateList(input);
    FcitxCandidateWordSetPageSize(candList, config->iMaxCandWord);
    FcitxUICloseInputWindow(instance);
    strcpy(FcitxInputStateGetRawInputBuffer(input), libpinyin->buf);
    FcitxInputStateSetRawInputBufferSize(input, strlen(libpinyin->buf));
    FcitxInputStateSetShowCursor(input, true);
    FcitxInputStateSetClientCursorPos(input, 0);
    
    if (libpinyin->type == LPT_Zhuyin && libpinyin->owner->config.zhuyinLayout != FCITX_ZHUYIN_ET26)
        FcitxCandidateWordSetChooseAndModifier(candList, "1234567890", FcitxKeyState_Alt);
    else
        FcitxCandidateWordSetChoose(candList, "1234567890");
    
    char* sentence = NULL;
    
    pinyin_guess_sentence(libpinyin->inst);
    pinyin_get_sentence(libpinyin->inst, &sentence);
    
    if (sentence) {
        FcitxCandidateWord candWord;
        FcitxLibpinyinCandWord* pyCand = (FcitxLibpinyinCandWord*) fcitx_utils_malloc0(sizeof(FcitxLibpinyinCandWord));
        pyCand->issentence = true;
        pyCand->token = 0;
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
    
    TokenVector array = g_array_new(FALSE, FALSE, sizeof(phrase_token_t));
    pinyin_get_candidates(libpinyin->inst, LibpinyinGetOffset(libpinyin), array);
    int i = 0;
    for (i = 0 ; i < array->len; i ++) {
        phrase_token_t token = g_array_index(array, phrase_token_t, i);
        char* tokenstring = NULL;
        pinyin_translate_token(libpinyin->inst, token, &tokenstring);
        
        if (tokenstring) {
            if (sentence && strcmp(sentence, tokenstring) == 0)
                continue;
            
            FcitxCandidateWord candWord;
            FcitxLibpinyinCandWord* pyCand = (FcitxLibpinyinCandWord*) fcitx_utils_malloc0(sizeof(FcitxLibpinyinCandWord));
            pyCand->issentence = false;
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
    
    g_array_free(array, false);
    
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
        char* sentence = NULL;
        pinyin_get_sentence(libpinyin->inst, &sentence);
        if (sentence) {
            strcpy(FcitxInputStateGetOutputString(input), sentence);
            pinyin_train(libpinyin->inst);
        }
        else
            strcpy(FcitxInputStateGetOutputString(input), "");
        
        return IRV_COMMIT_STRING;
    }
    else {
        pinyin_choose_candidate(libpinyin->inst, LibpinyinGetOffset(libpinyin), pyCand->token);
        char* tokenstring = NULL;
        pinyin_translate_token(libpinyin->inst, pyCand->token, &tokenstring);
        
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
            pinyin_get_sentence(libpinyin->inst, &sentence);
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
    libpinyin->inst = pinyin_alloc_instance(libpinyinaddon->context);
    libpinyin->fixed_string = g_array_new(FALSE, FALSE, sizeof(FcitxLibpinyinFixed));
    libpinyin->type = type;
    libpinyin->owner = libpinyinaddon;
    return libpinyin;
}

void FcitxLibpinyinDelete(FcitxLibpinyin* libpinyin)
{
    pinyin_free_instance(libpinyin->inst);
    g_array_free(libpinyin->fixed_string, FALSE);
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
    
    char* user_path = NULL;
    FILE* fp = FcitxXDGGetFileUserWithPrefix("libpinyin", "data/.place_holder", "w", NULL);
    if (fp)
        fclose(fp);
    FcitxXDGGetFileUserWithPrefix("libpinyin", "data", NULL, &user_path);
    FcitxLog(INFO, "Libpinyin storage path %s", user_path);
    libpinyinaddon->context = pinyin_init(LIBPINYIN_PKGDATADIR "/data", user_path);
    libpinyinaddon->pinyin = FcitxLibpinyinNew(libpinyinaddon, LPT_Pinyin);
    libpinyinaddon->shuangpin = FcitxLibpinyinNew(libpinyinaddon, LPT_Shuangpin);
    libpinyinaddon->zhuyin = FcitxLibpinyinNew(libpinyinaddon, LPT_Zhuyin);
    
    free(user_path);
    
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
                    "zh_CN"
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
                    "zh_CN"
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
                    "zh_CN"
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
    pinyin_fini(libpinyin->context);
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

    FILE *fp = FcitxXDGGetFileUserWithPrefix("conf", "fcitx-libpinyin.config", "rt", NULL);

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
    pinyin_set_chewing_scheme(libpinyinaddon->context, FcitxLibpinyinTransZhuyinLayout(config->zhuyinLayout));
    pinyin_set_double_pinyin_scheme(libpinyinaddon->context, FcitxLibpinyinTransShuangpinScheme(config->spScheme));
    PinyinCustomSettings settings;
    for (int i = 0; i <= FCITX_CR_LAST; i ++) {
        settings.set_use_corrections(FcitxLibpinyinTransCorrection((FCITX_CORRECTION) i), (bool) config->cr[i]);
    }
    for (int i = 0; i <= FCITX_AMB_LAST; i ++) {
        settings.set_use_ambiguities(FcitxLibpinyinTransAmbiguity((FCITX_AMBIGUITY) i), (bool) config->amb[i]);
    }
    settings.set_use_incomplete((bool) config->incomplete);
    pinyin_set_options(libpinyinaddon->context, &settings);
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
    FILE *fp = FcitxXDGGetFileUserWithPrefix("conf", "fcitx-libpinyin.config", "wt", NULL);
    FcitxConfigSaveConfigFileFp(fp, &fs->gconfig, configDesc);
    if (fp)
        fclose(fp);
}

void FcitxLibpinyinSave(void* arg)
{
    FcitxLibpinyin* libpinyin = (FcitxLibpinyin*) arg;
    pinyin_save(libpinyin->owner->context);
}


// kate: indent-mode cstyle; space-indent on; indent-width 0;
