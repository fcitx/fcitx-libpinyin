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

#ifdef __cplusplus
#define __EXPORT_API extern "C"
#else
#define __EXPORT_API
#endif

#define _(x) gettext(x)

class FcitxWindowHandler;

/*
 * the reason that not using libpinyin enum here
 * 1. libpinyin seems cannot make enum stable (from their header)
 * 2. the range is not continous, so make a translate layer
 */
enum FCITX_ZHUYIN_LAYOUT {
    FCITX_ZHUYIN_STANDARD = 0,
    FCITX_ZHUYIN_HSU      = 1,
    FCITX_ZHUYIN_IBM      = 2,
    FCITX_ZHUYIN_GIN_YIEH = 3,
    FCITX_ZHUYIN_ET       = 4,
    FCITX_ZHUYIN_ET26     = 5,
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
    FCITX_CR_VU,
    FCITX_CR_LAST = FCITX_CR_VU
};

enum FCITX_AMBIGUITY {
    FCITX_AMB_CiChi,
    FCITX_AMB_ChiCi,
    FCITX_AMB_ZiZhi,
    FCITX_AMB_ZhiZi,
    FCITX_AMB_SiShi,
    FCITX_AMB_ShiSi,
    FCITX_AMB_LeNe,
    FCITX_AMB_NeLe,
    FCITX_AMB_FoHe,
    FCITX_AMB_HeFo,
    FCITX_AMB_LeRi,
    FCITX_AMB_RiLe,
    FCITX_AMB_KeGe,
    FCITX_AMB_GeKe,
    FCITX_AMB_AnAng,
    FCITX_AMB_AngAn,
    FCITX_AMB_EnEng,
    FCITX_AMB_EngEn,
    FCITX_AMB_InIng,
    FCITX_AMB_IngIn,
    FCITX_AMB_LAST = FCITX_AMB_IngIn
};

struct FcitxLibpinyinConfig
{
    FcitxGenericConfig gconfig;
    FCITX_ZHUYIN_LAYOUT zhuyinLayout;
    FCITX_SHUANGPIN_SCHEME spScheme;
    boolean amb[FCITX_AMB_LAST + 1];
    boolean cr[FCITX_CR_LAST + 1];
    boolean incomplete;
};

#define BUF_SIZE 4096

CONFIG_BINDING_DECLARE(FcitxLibpinyinConfig);
__EXPORT_API void* FcitxLibpinyinCreate(FcitxInstance* instance);
__EXPORT_API void FcitxLibpinyinDestroy(void* arg);
__EXPORT_API INPUT_RETURN_VALUE FcitxLibpinyinDoInput(void* arg, FcitxKeySym sym, unsigned int state);
__EXPORT_API INPUT_RETURN_VALUE FcitxLibpinyinGetCandWords (void *arg);
__EXPORT_API INPUT_RETURN_VALUE FcitxLibpinyinGetCandWord (void *arg, FcitxCandidateWord* candWord);
__EXPORT_API boolean FcitxLibpinyinInit(void*);
__EXPORT_API void ReloadConfigFcitxLibpinyin(void*);

enum LIBPINYIN_TYPE {
    LPT_Pinyin,
    LPT_Zhuyin,
    LPT_Shuangpin
};

struct _FcitxLibpinyin;

typedef struct _FcitxLibpinyinAddonInstance {
    FcitxLibpinyinConfig config;
    
    pinyin_context_t* context;
    
    struct _FcitxLibpinyin* pinyin;
    struct _FcitxLibpinyin* shuangpin;
    struct _FcitxLibpinyin* zhuyin;
    FcitxInstance* owner;
} FcitxLibpinyinAddonInstance;

typedef struct _FcitxLibpinyin
{
    pinyin_instance_t* inst;
    
    GArray* fixed_string;
    
    char buf[MAX_USER_INPUT + 1];
    int cursor_pos;
    LIBPINYIN_TYPE type;
    
    FcitxLibpinyinAddonInstance* owner;
} FcitxLibpinyin;

#endif
// kate: indent-mode cstyle; space-indent on; indent-width 0;
