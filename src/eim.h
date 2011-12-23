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
struct FcitxLibpinyinConfig
{
    FcitxGenericConfig gconfig;
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

typedef struct FcitxLibpinyin
{
    FcitxLibpinyinConfig config;
    
    pinyin_context_t* context;
    pinyin_instance_t* inst;
    
    GArray* fixed_string;
    
    char buf[MAX_USER_INPUT + 1];
    int cursor_pos;
    
    FcitxInstance* owner;
} FcitxLibpinyin;

#endif
// kate: indent-mode cstyle; space-indent on; indent-width 0;
