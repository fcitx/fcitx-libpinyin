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

#include "eim.h"

CONFIG_BINDING_BEGIN(FcitxLibpinyinConfig)
CONFIG_BINDING_REGISTER("Pinyin", "Incomplete", incomplete)
CONFIG_BINDING_REGISTER("Zhuyin", "Layout", zhuyinLayout)
CONFIG_BINDING_REGISTER("Zhuyin", "PrevPage", hkPrevPage)
CONFIG_BINDING_REGISTER("Zhuyin", "NextPage", hkNextPage)
CONFIG_BINDING_REGISTER("Shuangpin", "Scheme", spScheme)
CONFIG_BINDING_REGISTER("Correction", "VU", cr[FCITX_CR_VU])
CONFIG_BINDING_REGISTER("Ambiguity", "CiChi", amb[FCITX_AMB_CiChi])
CONFIG_BINDING_REGISTER("Ambiguity", "ChiCi", amb[FCITX_AMB_ChiCi])
CONFIG_BINDING_REGISTER("Ambiguity", "ZiZhi", amb[FCITX_AMB_ZiZhi])
CONFIG_BINDING_REGISTER("Ambiguity", "ZhiZi", amb[FCITX_AMB_ZhiZi])
CONFIG_BINDING_REGISTER("Ambiguity", "SiShi", amb[FCITX_AMB_SiShi])
CONFIG_BINDING_REGISTER("Ambiguity", "ShiSi", amb[FCITX_AMB_ShiSi])
CONFIG_BINDING_REGISTER("Ambiguity", "LeNe", amb[FCITX_AMB_LeNe])
CONFIG_BINDING_REGISTER("Ambiguity", "NeLe", amb[FCITX_AMB_NeLe])
CONFIG_BINDING_REGISTER("Ambiguity", "FoHe", amb[FCITX_AMB_FoHe])
CONFIG_BINDING_REGISTER("Ambiguity", "HeFo", amb[FCITX_AMB_HeFo])
CONFIG_BINDING_REGISTER("Ambiguity", "LeRi", amb[FCITX_AMB_LeRi])
CONFIG_BINDING_REGISTER("Ambiguity", "RiLe", amb[FCITX_AMB_RiLe])
CONFIG_BINDING_REGISTER("Ambiguity", "KeGe", amb[FCITX_AMB_KeGe])
CONFIG_BINDING_REGISTER("Ambiguity", "GeKe", amb[FCITX_AMB_GeKe])
CONFIG_BINDING_REGISTER("Ambiguity", "AnAng", amb[FCITX_AMB_AnAng])
CONFIG_BINDING_REGISTER("Ambiguity", "AngAn", amb[FCITX_AMB_AngAn])
CONFIG_BINDING_REGISTER("Ambiguity", "EnEng", amb[FCITX_AMB_EnEng])
CONFIG_BINDING_REGISTER("Ambiguity", "EngEn", amb[FCITX_AMB_EngEn])
CONFIG_BINDING_REGISTER("Ambiguity", "InIng", amb[FCITX_AMB_InIng])
CONFIG_BINDING_REGISTER("Ambiguity", "IngIn", amb[FCITX_AMB_IngIn])
CONFIG_BINDING_END()
