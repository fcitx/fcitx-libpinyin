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

#include <QProcess>
#include <QTemporaryFile>

#include <fcitx-utils/utils.h>
#include <fcitx-config/xdg.h>

#include "scelconverter.h"
#include "guicommon.h"

ScelConverter::ScelConverter(QObject* parent): QObject(parent)
    ,m_file(getTempdir().append("/fcitx_dictmanager_XXXXXX"))
{
}

void ScelConverter::removeTempFile()
{
    QFile::remove(m_fromFile);
}

void ScelConverter::convert(const QString& from, const QString& to, bool removeOriginFile)
{
    if (!m_file.open()) {
        emit message(QMessageBox::Warning, _("Create temporary file failed."));
        emit finished(false);
        return;
    } else {
        emit message(QMessageBox::Information, _("Temporary file created."));
    }

    m_file.close();
    m_file.setAutoRemove(false);

    m_fromFile = from;
    if (removeOriginFile) {
        connect(this, SIGNAL(finished(bool)), this, SLOT(removeTempFile()));
    }

    char* path = fcitx_utils_get_fcitx_path_with_filename("bindir", "scel2org");
    QStringList arguments;
    arguments << "-a" << "-o" << m_file.fileName() << from;
    m_process.start(path, arguments);
    m_process.closeReadChannel(QProcess::StandardError);
    m_process.closeReadChannel(QProcess::StandardOutput);
    connect(&m_process, SIGNAL(finished(int, QProcess::ExitStatus)), SLOT(finished(int,QProcess::ExitStatus)));

    m_name = to;
}

void ScelConverter::finished(int exitCode, QProcess::ExitStatus status)
{
    if (status == QProcess::CrashExit) {
        emit message(QMessageBox::Critical, _("Converter crashed."));
        emit finished(false);
        return;
    }

    if (exitCode != 0) {
        emit message(QMessageBox::Warning, _("Convert failed."));
        emit finished(false);
    }

    char* fullName;
    FcitxXDGMakeDirUser("libpinyin/importdict");
    FcitxXDGGetFileUserWithPrefix("libpinyin/importdict", m_name.toLocal8Bit().constData(), NULL, &fullName);

    if (QFile::rename(m_file.fileName(), QString::fromLocal8Bit(fullName))) {
        emit finished(true);
    } else {
        QFile::remove(m_file.fileName());
        emit message(QMessageBox::Warning, _("Rename failed."));
        emit finished(false);
    }

    free(fullName);
}


