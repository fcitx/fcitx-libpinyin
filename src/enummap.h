/***************************************************************************
 *   Copyright (C) 2011~2011 by CSSlayer                                   *
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

#ifndef ENUMMAP_H
#define ENUMMAP_H

#include <pinyin.h>
#include "eim.h"

ZhuyinScheme FcitxLibPinyinTransZhuyinLayout(FCITX_ZHUYIN_LAYOUT layout);
DoublePinyinScheme FcitxLibPinyinTransShuangpinScheme(FCITX_SHUANGPIN_SCHEME scheme);
PinyinAmbiguity2 FcitxLibPinyinTransAmbiguity(FCITX_AMBIGUITY ambiguity);
PinyinCorrection2 FcitxLibPinyinTransCorrection(FCITX_CORRECTION correction);
int FcitxLibPinyinTransDictionary(FCITX_DICTIONARY dict);
int FcitxLibPinyinTransZhuyinDictionary(FCITX_ZHUYIN_DICTIONARY dict);
sort_option_t FcitxLibPinyinTransSortOption(FCITX_LIBPINYIN_SORT sort);

#endif
