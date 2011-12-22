/***************************************************************************
 *   Copyright (C) 2010~2010 by CSSlayer                                   *
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


#define FCITX_LIBPINYIN_MAX(x, y) ((x) > (y)? (x) : (y))

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

CONFIG_DESC_DEFINE(GetLibpinyinConfigDesc, "fcitx-libpinyin.desc")

boolean LoadLibpinyinConfig(FcitxLibpinyinConfig* fs);
static void SaveLibpinyinConfig(FcitxLibpinyinConfig* fs);
static void ConfigLibpinyin(FcitxLibpinyin* libpinyin);

/**
 * @brief Reset the status.
 *
 **/
__EXPORT_API
void FcitxLibpinyinReset (void* arg)
{
    FcitxLibpinyin* libpinyin = (FcitxLibpinyin*) arg;
    libpinyin->buf[0] = '\0';
    libpinyin->CursorPos = 0;
    libpinyin->offset = 0;
    pinyin_reset(libpinyin->inst);
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
    FcitxInputState* input = FcitxInstanceGetInputState(libpinyin->owner);
    
    if (FcitxHotkeyIsHotKeySimple(sym, state))
    {
        if (FcitxHotkeyIsHotKeyLAZ(sym, state) || sym == '\'')
        {
            if (strlen(libpinyin->buf) < MAX_USER_INPUT)
            {
                size_t len = strlen(libpinyin->buf);
                if (libpinyin->buf[libpinyin->CursorPos] != 0)
                {
                    memmove(libpinyin->buf + libpinyin->CursorPos + 1, libpinyin->buf + libpinyin->CursorPos, len - libpinyin->CursorPos);
                }
                libpinyin->buf[len + 1] = 0;
                libpinyin->buf[libpinyin->CursorPos] = (char) (sym & 0xff);
                libpinyin->CursorPos ++;
                
                len = pinyin_parse_more_full_pinyins(libpinyin->inst, libpinyin->buf);
                
                if (len == 0 && strlen(libpinyin->buf) == 1)
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
            if (FcitxHotkeyIsHotKey(sym, state, FCITX_BACKSPACE))
            {
                if (libpinyin->CursorPos > 0)
                    libpinyin->CursorPos -- ;
                else
                    return IRV_DO_NOTHING;
            }
            size_t len = strlen(libpinyin->buf);
            if (libpinyin->CursorPos == (int)len)
                return IRV_DO_NOTHING;
            memmove(libpinyin->buf + libpinyin->CursorPos, libpinyin->buf + libpinyin->CursorPos + 1, len - libpinyin->CursorPos - 1);
            libpinyin->buf[strlen(libpinyin->buf) - 1] = 0;
            pinyin_parse_more_full_pinyins(libpinyin->inst, libpinyin->buf);
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
                if (libpinyin->CursorPos > 0)
                {
                    libpinyin->CursorPos -- ;
                    pinyin_parse_more_full_pinyins(libpinyin->inst, libpinyin->buf);
                    return IRV_DISPLAY_CANDWORDS;
                }

                return IRV_DO_NOTHING;
            }
            else if (FcitxHotkeyIsHotKey(sym, state, FCITX_RIGHT))
            {
                size_t len = strlen(libpinyin->buf);
                if (libpinyin->CursorPos < (int) len)
                {
                    libpinyin->CursorPos ++ ;
                    pinyin_parse_more_full_pinyins(libpinyin->inst, libpinyin->buf);
                    return IRV_DISPLAY_CANDWORDS;
                }
                return IRV_DO_NOTHING;
            }
            else if (FcitxHotkeyIsHotKey(sym, state, FCITX_HOME))
            {
                if ( libpinyin->CursorPos != 0)
                {
                    libpinyin->CursorPos = 0;
                    pinyin_parse_more_full_pinyins(libpinyin->inst, libpinyin->buf);
                    return IRV_DISPLAY_CANDWORDS;
                }
                return IRV_DO_NOTHING;
            }
            else if (FcitxHotkeyIsHotKey(sym, state, FCITX_END))
            {
                size_t len = strlen(libpinyin->buf);
                if (libpinyin->CursorPos != (int) len)
                {
                    libpinyin->CursorPos = len ;
                    pinyin_parse_more_full_pinyins(libpinyin->inst, libpinyin->buf);
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
    FcitxInstance* instance = libpinyin->owner;
    FcitxInputState* input = FcitxInstanceGetInputState(instance);
    FcitxGlobalConfig* config = FcitxInstanceGetGlobalConfig(libpinyin->owner);
    FcitxCandidateWordSetPageSize(FcitxInputStateGetCandidateList(input), config->iMaxCandWord);
    FcitxUICloseInputWindow(instance);
    strcpy(FcitxInputStateGetRawInputBuffer(input), libpinyin->buf);
    FcitxInputStateSetRawInputBufferSize(input, strlen(libpinyin->buf));
    
    FcitxInputStateSetClientCursorPos(input, 0);
    FcitxInputStateSetCursorPos(input, libpinyin->CursorPos);
    
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
        
        
        for (int i = 0; i < libpinyin->inst->m_pinyin_keys->len; i ++)
        {
            PinyinKey* pykey = &g_array_index(libpinyin->inst->m_pinyin_keys, PinyinKey, i);
            FcitxMessagesAddMessageAtLast(FcitxInputStateGetPreedit(input), MSG_INPUT, "%s ", pykey->get_key_string());
        }
        
        FcitxMessagesAddMessageAtLast(FcitxInputStateGetClientPreedit(input), MSG_INPUT, "%s", sentence);
    }
    else {
        FcitxMessagesAddMessageAtLast(FcitxInputStateGetClientPreedit(input), MSG_INPUT, "%s", libpinyin->buf);
        FcitxMessagesAddMessageAtLast(FcitxInputStateGetPreedit(input), MSG_INPUT, "%s", libpinyin->buf);
    }
    
    TokenVector array = g_array_new(FALSE, FALSE, sizeof(phrase_token_t));
    pinyin_get_candidates(libpinyin->inst, libpinyin->offset, array);
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
    FcitxInstance* instance = libpinyin->owner;
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
        pinyin_choose_candidate(libpinyin->inst, libpinyin->offset, pyCand->token);
        return IRV_DISPLAY_CANDWORDS;
    }
    return IRV_TO_PROCESS;
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
    FcitxLibpinyin* libpinyin = (FcitxLibpinyin*) fcitx_utils_malloc0(sizeof(FcitxLibpinyin));
    FcitxAddon* addon = FcitxAddonsGetAddonByName(FcitxInstanceGetAddons(instance), "fcitx-libpinyin");
    bindtextdomain("fcitx-libpinyin", LOCALEDIR);
    libpinyin->owner = instance;

    if (!LoadLibpinyinConfig(&libpinyin->config))
    {
        free(libpinyin);
        return NULL;
    }
    
    ConfigLibpinyin(libpinyin);
    
    char* user_path = NULL;
    FcitxXDGGetFileUserWithPrefix("libpinyin", "data", NULL, &user_path);    
    libpinyin->context = pinyin_init(LIBPINYIN_PKGDATADIR "/data", user_path);
    libpinyin->inst = pinyin_alloc_instance(libpinyin->context);
    
    free(user_path);
    
    FcitxInstanceRegisterIM(instance,
                    libpinyin,
                    "pinyin-libpinyin",
                    _("Pinyin (LibPinyin)"),
                    "pinyin",
                    FcitxLibpinyinInit,
                    FcitxLibpinyinReset,
                    FcitxLibpinyinDoInput,
                    FcitxLibpinyinGetCandWords,
                    NULL,
                    NULL,
                    ReloadConfigFcitxLibpinyin,
                    NULL,
                    5,
                    "zh_CN"
                   );
    
    /*
    FcitxInstanceRegisterIM(instance,
                    libpinyin,
                    "chewing-libpinyin",
                    _("Chewing"),
                    "chewing",
                    FcitxLibpinyinChewingInit,
                    FcitxLibpinyinReset,
                    FcitxLibpinyinDoInput,
                    FcitxLibpinyinGetCandWords,
                    NULL,
                    NULL,
                    ReloadConfigFcitxLibpinyin,
                    NULL,
                    NULL,
                    5,
                    "zh_TW"
                   ); */

    return libpinyin;
}

/**
 * @brief Destroy the input method while unload it.
 *
 * @return int
 **/
__EXPORT_API
void FcitxLibpinyinDestroy (void* arg)
{
    FcitxLibpinyin* libpinyin = (FcitxLibpinyin*) arg;
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

void ConfigLibpinyin(FcitxLibpinyin* libpinyin)
{
    FcitxInstance* instance = libpinyin->owner;
    FcitxGlobalConfig* config = FcitxInstanceGetGlobalConfig(instance);
    FcitxLibpinyinConfig *fs = &libpinyin->config;
}

__EXPORT_API void ReloadConfigFcitxLibpinyin(void* arg)
{
    FcitxLibpinyin* libpinyin = (FcitxLibpinyin*) arg;
    LoadLibpinyinConfig(&libpinyin->config);
    ConfigLibpinyin(libpinyin);
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

// kate: indent-mode cstyle; space-indent on; indent-width 0;
