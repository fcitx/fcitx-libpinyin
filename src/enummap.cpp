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

#include "enummap.h"

PinyinAmbiguity FcitxLibpinyinTransAmbiguity(FCITX_AMBIGUITY ambiguity)
{
    switch(ambiguity) {
        case FCITX_AMB_CiChi:
            return PINYIN_AmbCiChi;
        case FCITX_AMB_ChiCi:
            return PINYIN_AmbChiCi;
        case FCITX_AMB_ZiZhi:
            return PINYIN_AmbZiZhi;
        case FCITX_AMB_ZhiZi:
            return PINYIN_AmbZhiZi;
        case FCITX_AMB_SiShi:
            return PINYIN_AmbSiShi;
        case FCITX_AMB_ShiSi:
            return PINYIN_AmbShiSi;
        case FCITX_AMB_LeNe:
            return PINYIN_AmbLeNe;
        case FCITX_AMB_NeLe:
            return PINYIN_AmbNeLe;
        case FCITX_AMB_FoHe:
            return PINYIN_AmbFoHe;
        case FCITX_AMB_HeFo:
            return PINYIN_AmbHeFo;
        case FCITX_AMB_LeRi:
            return PINYIN_AmbLeRi;
        case FCITX_AMB_RiLe:
            return PINYIN_AmbRiLe;
        case FCITX_AMB_KeGe:
            return PINYIN_AmbKeGe;
        case FCITX_AMB_GeKe:
            return PINYIN_AmbGeKe;
        case FCITX_AMB_AnAng:
            return PINYIN_AmbAnAng;
        case FCITX_AMB_AngAn:
            return PINYIN_AmbAngAn;
        case FCITX_AMB_EnEng:
            return PINYIN_AmbEnEng;
        case FCITX_AMB_EngEn:
            return PINYIN_AmbEngEn;
        case FCITX_AMB_InIng:
            return PINYIN_AmbInIng;
        case FCITX_AMB_IngIn:
            return PINYIN_AmbIngIn;
        default:
            return PINYIN_AmbAny;
    }
}

PinyinCorrection FcitxLibpinyinTransCorrection(FCITX_CORRECTION correction)
{
    switch(correction) {
        case FCITX_CR_VU:
            return PINYIN_CorrectVtoU;
        default:
            return PINYIN_CorrectVtoU;
    }
}

PinyinShuangPinScheme FcitxLibpinyinTransShuangpinScheme(FCITX_SHUANGPIN_SCHEME scheme)
{
    switch(scheme) {
        case FCITX_SHUANG_PIN_ZRM:
            return SHUANG_PIN_ZRM;
        case FCITX_SHUANG_PIN_MS:
            return SHUANG_PIN_MS;
        case FCITX_SHUANG_PIN_ZIGUANG:
            return SHUANG_PIN_ZIGUANG;
        case FCITX_SHUANG_PIN_ABC:
            return SHUANG_PIN_ABC;
        case FCITX_SHUANG_PIN_PYJJ:
            return SHUANG_PIN_PYJJ;
        case FCITX_SHUANG_PIN_XHE:
            return SHUANG_PIN_XHE;
        default:
            return SHUANG_PIN_ZRM;
    }
}

PinyinZhuYinScheme FcitxLibpinyinTransZhuyinLayout(FCITX_ZHUYIN_LAYOUT layout)
{
    switch(layout) {
        case FCITX_ZHUYIN_STANDARD:
            return ZHUYIN_STANDARD;
        case FCITX_ZHUYIN_HSU:
            return ZHUYIN_HSU;
        case FCITX_ZHUYIN_IBM:
            return ZHUYIN_IBM;
        case FCITX_ZHUYIN_GIN_YIEH:
            return ZHUYIN_GIN_YIEH;
        case FCITX_ZHUYIN_ET:
            return ZHUYIN_ET;
        case FCITX_ZHUYIN_ET26:
            return ZHUYIN_ET26;
        default:
            return ZHUYIN_ZHUYIN;
    }
}
