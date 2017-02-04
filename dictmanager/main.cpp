/***************************************************************************
 *   Copyright (C) 2013~2013 by CSSlayer                                   *
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

#include "main.h"
#include "dictmanager.h"

LibPinyinDictManagerPlugin::LibPinyinDictManagerPlugin(QObject* parent)
    : FcitxQtConfigUIPlugin(parent)
{
}

QString LibPinyinDictManagerPlugin::name()
{
    return "libpinyin-dictmanager";
}
QStringList LibPinyinDictManagerPlugin::files()
{
    QStringList list;
    list << "libpinyin-dictmanager";
    return list;
}
QString LibPinyinDictManagerPlugin::domain()
{
    return "fcitx-libpinyin";
}
FcitxQtConfigUIWidget* LibPinyinDictManagerPlugin::create(const QString& key)
{
    if (key == "libpinyin-dictmanager") {
        return new DictManager;
    }
    return 0;
}
