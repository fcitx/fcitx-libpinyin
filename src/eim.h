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

#ifndef EIM_H
#define EIM_H

#include <fcitx/ime.h>
#include <fcitx-config/fcitx-config.h>
#include <fcitx/instance.h>
#include <fcitx/candidate.h>
#include <pinyin.h>
#include <vector>
#include <string>
#include "bus.h"
#include "common.h"

#define MAX_PINYIN_INPUT 60

#define _(x) dgettext("fcitx-libpinyin", (x))

class FcitxWindowHandler;

/*
 * the reason that not using libpinyin enum here
 * 1. libpinyin seems cannot make enum stable (from their header)
 * 2. the range is not continous, so make a translate layer
 */
enum FCITX_ZHUYIN_LAYOUT {
    FCITX_ZHUYIN_STANDARD = 0,
    FCITX_ZHUYIN_IBM      = 1,
    FCITX_ZHUYIN_GIN_YIEH = 2,
    FCITX_ZHUYIN_ET       = 3
};

enum FCITX_SHUANGPIN_SCHEME {
    FCITX_SHUANG_PIN_ZRM        = 0,
    FCITX_SHUANG_PIN_MS         = 1,
    FCITX_SHUANG_PIN_ZIGUANG    = 2,
    FCITX_SHUANG_PIN_ABC        = 3,
    FCITX_SHUANG_PIN_PYJJ       = 4,
    FCITX_SHUANG_PIN_XHE        = 5
};

enum FCITX_CORRECTION {
    FCITX_CR_V_U,
    FCITX_CR_GN_NG,
    FCITX_CR_MG_NG,
    FCITX_CR_IOU_IU,
    FCITX_CR_UEI_UI,
    FCITX_CR_UEN_UN,
    FCITX_CR_UE_VE,
    FCITX_CR_ON_ONG,
    FCITX_CR_LAST = FCITX_CR_ON_ONG
};

enum FCITX_DICTIONARY {
    FCITX_DICT_ART,
    FCITX_DICT_CULTURE,
    FCITX_DICT_ECONOMY,
    FCITX_DICT_GEOLOGY,
    FCITX_DICT_HISTORY,
    FCITX_DICT_LIFE,
    FCITX_DICT_NATURE,
    FCITX_DICT_PEOPLE,
    FCITX_DICT_SCIENCE,
    FCITX_DICT_SOCIETY,
    FCITX_DICT_SPORT,
    FCITX_DICT_TECHNOLOGY,
    FCITX_DICT_LAST = FCITX_DICT_TECHNOLOGY
};

enum FCITX_ZHUYIN_DICTIONARY {
    FCITX_ZHUYIN_DICT_ART,
    FCITX_ZHUYIN_DICT_CULTURE,
    FCITX_ZHUYIN_DICT_ECONOMY,
    FCITX_ZHUYIN_DICT_GEOLOGY,
    FCITX_ZHUYIN_DICT_HISTORY,
    FCITX_ZHUYIN_DICT_LIFE,
    FCITX_ZHUYIN_DICT_NATURE,
    FCITX_ZHUYIN_DICT_SCITECH,
    FCITX_ZHUYIN_DICT_SOCIETY,
    FCITX_ZHUYIN_DICT_SPORT,
    FCITX_ZHUYIN_DICT_LAST = FCITX_ZHUYIN_DICT_SPORT
};

enum FCITX_AMBIGUITY {
    FCITX_AMB_CiChi,
    FCITX_AMB_ZiZhi,
    FCITX_AMB_SiShi,
    FCITX_AMB_LeNe,
    FCITX_AMB_FoHe,
    FCITX_AMB_LeRi,
    FCITX_AMB_GeKe,
    FCITX_AMB_AnAng,
    FCITX_AMB_EnEng,
    FCITX_AMB_InIng,
    FCITX_AMB_LAST = FCITX_AMB_InIng
};

enum FCITX_ZHUYIN_MODIFIERS {
    FZM_Ctrl = 0,
    FZM_Alt = 1,
    FZM_Shift = 2,
};

struct FcitxLibPinyinConfig
{
    FcitxGenericConfig gconfig;
    FCITX_ZHUYIN_LAYOUT zhuyinLayout;
    FCITX_SHUANGPIN_SCHEME spScheme;
    FCITX_ZHUYIN_MODIFIERS candidateModifiers;
    boolean amb[FCITX_AMB_LAST + 1];
    boolean cr[FCITX_CR_LAST + 1];
    boolean dict[FCITX_DICT_LAST + 1];
    boolean dict_zhuyin[FCITX_ZHUYIN_DICT_LAST + 1];
    boolean incomplete;
    boolean zhuyinIncomplete;
    boolean bTraditionalDataForPinyin;
    boolean bSimplifiedDataForZhuyin;
    boolean useTone;
    FcitxHotkey hkPrevPage[2];
    FcitxHotkey hkNextPage[2];
};

#define BUF_SIZE 4096

CONFIG_BINDING_DECLARE(FcitxLibPinyinConfig);
void* FcitxLibPinyinCreate(FcitxInstance* instance);
void FcitxLibPinyinDestroy(void* arg);
INPUT_RETURN_VALUE FcitxLibPinyinDoInput(void* arg, FcitxKeySym sym, unsigned int state);
INPUT_RETURN_VALUE FcitxLibPinyinGetCandWords (void *arg);
INPUT_RETURN_VALUE FcitxLibPinyinGetCandWord (void *arg, FcitxCandidateWord* candWord);
boolean FcitxLibPinyinInit(void*);
void FcitxLibPinyinReloadConfig(void*);

class FcitxLibPinyin;

typedef struct _FcitxLibPinyinAddonInstance {
    FcitxLibPinyinConfig config;

    pinyin_context_t* pinyin_context;
    pinyin_context_t* zhuyin_context;

    FcitxLibPinyin* pinyin;
    FcitxLibPinyin* shuangpin;
    FcitxLibPinyin* zhuyin;
    FcitxInstance* owner;
    FcitxLibPinyinBus* bus;
} FcitxLibPinyinAddonInstance;

class FcitxLibPinyin
{
public:
    FcitxLibPinyin(FcitxLibPinyinAddonInstance* libpinyinaddon, LIBPINYIN_TYPE type);
    ~FcitxLibPinyin();

    void import();
    void clearData(int type);

    int offset() const;
    int pinyinOffset() const;
    void reset();
    size_t parse(const char* str);
    INPUT_RETURN_VALUE doInput(FcitxKeySym sym, unsigned int state);
    void load();
    INPUT_RETURN_VALUE getCandWords();
    INPUT_RETURN_VALUE getCandWord(FcitxCandidateWord* candWord);
    std::string sentence(guint8 index);
    void init();
    void updatePreedit(const std::string &sentence);
    void savePinyinWord(const char *str);
    void save();

private:
    pinyin_instance_t* m_inst;
    std::vector<std::pair<int, int> > m_fixedString;
    std::string m_buf;
    int m_cursorPos;
    int m_parsedLen;
    LIBPINYIN_TYPE m_type;
    FcitxLibPinyinAddonInstance* m_owner;
};

#endif
// kate: indent-mode cstyle; space-indent on; indent-width 0;
