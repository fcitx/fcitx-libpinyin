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

#include <QTemporaryFile>

#include "filedownloader.h"
#include "guicommon.h"

FileDownloader::FileDownloader(QObject *parent) :
    QObject(parent)
{
}

FileDownloader::~FileDownloader()
{

}

void FileDownloader::download(const QUrl& url)
{
    if (!m_file.open()) {
        emit message(_("Create temporary file failed."));
        emit finished(false);
        return;
    } else {
        emit message(_("Temporary file created."));
    }

    QNetworkRequest request(url);
    request.setRawHeader("Referer", QString("http://%1").arg(url.host()).toAscii());
    m_reply = m_WebCtrl.get(request);

    if (!m_reply) {
        emit message(_("Failed to create request."));
        emit finished(false);
        return;
    }

    connect(m_reply, SIGNAL(readyRead()), SLOT(readyToRead()));
    connect(m_reply, SIGNAL(finished()), SLOT(finished()));
}

QString FileDownloader::fileName()
{
    return m_file.fileName();
}

void FileDownloader::readyToRead()
{
    m_file.write(m_reply->readAll());
}

void FileDownloader::finished()
{
    m_file.close();
    m_file.setAutoRemove(false);
    emit message(_("Download Finished"));
    emit finished(true);
}
