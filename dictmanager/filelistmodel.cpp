/***************************************************************************
 *   Copyright (C) 2013~2013 by CSSlayer                                   *
 *   wengxt@gmail.com                                                      *
 *                                                                         *
 *  This program is free software: you can redistribute it and/or modify   *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation, either version 3 of the License, or      *
 *  (at your option) any later version.                                    *
 *                                                                         *
 *  This program is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *  You should have received a copy of the GNU General Public License      *
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.  *
 *                                                                         *
 ***************************************************************************/

#include <fcitx-config/xdg.h>

#include "common.h"
#include "filelistmodel.h"

FileListModel::FileListModel(QObject* parent): QAbstractListModel(parent)
    ,m_langType(LPLT_Simplified)
{
    loadFileList();
}

FileListModel::~FileListModel()
{

}

QString FileListModel::dictDir() const
{
    return m_langType == LPLT_Simplified ? "libpinyin/importdict" : "libpinyin/importdict_zhuyin";
}

int FileListModel::rowCount(const QModelIndex& parent) const
{
    return m_fileList.size();
}

QVariant FileListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= m_fileList.size())
        return QVariant();

    switch (role) {
    case Qt::DisplayRole:
        {
            const int length = dictDir().size();
            return m_fileList[index.row()].mid(length + 1, m_fileList[index.row()].size() - length - strlen(".txt") - 1);
        }
    case Qt::UserRole:
        return m_fileList[index.row()];
    default:
        break;
    }
    return QVariant();
}

void FileListModel::loadFileList()
{
    beginResetModel();
    m_fileList.clear();
    FcitxStringHashSet* files = FcitxXDGGetFiles(dictDir().toLocal8Bit().constData(), NULL, ".txt");

    HASH_SORT(files, fcitx_utils_string_hash_set_compare);
    HASH_FOREACH(f, files, FcitxStringHashSet) {
        m_fileList.append(QString::fromLocal8Bit(f->name).prepend(dictDir() + "/"));
    }
    fcitx_utils_free_string_hash_set(files);

    endResetModel();
}

int FileListModel::findFile(const QString& lastFileName)
{
    int idx = m_fileList.indexOf(lastFileName);
    if (idx < 0) {
        return 0;
    }
    return idx;
}
