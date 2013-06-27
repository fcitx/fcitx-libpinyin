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

#include <QFile>

#include "importer.h"
#include "common.h"
#include <pinyin.h>
#include <QDBusInterface>
#include <QDBusPendingCall>
#include <QDebug>
#include <fcitx-config/xdg.h>
#include <fcitx-utils/utf8.h>

Importer::Importer(QObject* parent): QObject(parent)
    ,m_connection(new FcitxQtConnection(this))
    ,m_running(false)
{
    m_connection->setAutoReconnect(true);

    connect(m_connection, SIGNAL(connected()), this, SLOT(onConnected()));
    connect(m_connection, SIGNAL(disconnected()), this, SLOT(onDisconnected()));
    m_connection->startConnection();
}

void Importer::onConnected()
{
    if (m_running) {
        realRun();
    }

}

void Importer::onDisconnected()
{
    if (m_running) {
        m_running = false;
        emit finished();
    }
}

Importer::~Importer()
{

}

void Importer::run()
{
    if (m_running) {
        return;
    }

    m_running = true;
    realRun();
}

void Importer::realRun()
{
    if (m_connection->isConnected()) {
        QDBusInterface iface(m_connection->serviceName(),
                             FCITX_LIBPINYIN_PATH,
                             FCITX_LIBPINYIN_INTERFACE,
                             *m_connection->connection());
        QDBusPendingCall call1 = iface.asyncCall("ImportDict");
        call1.waitForFinished();

        m_running = false;
        emit finished();
    }
}

void Importer::clearDict(int type)
{
    if (m_connection->isConnected()) {
        QDBusInterface iface(m_connection->serviceName(),
                             FCITX_LIBPINYIN_PATH,
                             FCITX_LIBPINYIN_INTERFACE,
                             *m_connection->connection());
        QDBusPendingCall call1 = iface.asyncCall("ClearDict", type);
        call1.waitForFinished();
    }
}

