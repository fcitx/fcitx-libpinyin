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
#include <QDBusInterface>
#include <QDBusPendingCall>
#include <QDebug>

#include <fcitx-config/xdg.h>
#include <fcitx-utils/utf8.h>

#include "importer.h"
#include "common.h"

Importer::Importer(QObject* parent): QObject(parent)
    ,m_connection(new FcitxQtConnection(this))
    ,m_running(false)
    ,m_iface(0)
{
    m_connection->setAutoReconnect(true);

    connect(m_connection, SIGNAL(connected()), this, SLOT(onConnected()));
    connect(m_connection, SIGNAL(disconnected()), this, SLOT(onDisconnected()));
    m_connection->startConnection();
}

Importer::~Importer()
{

}

void Importer::onConnected()
{
    m_iface = new QDBusInterface(m_connection->serviceName(),
                                 FCITX_LIBPINYIN_PATH,
                                 FCITX_LIBPINYIN_INTERFACE,
                                 *m_connection->connection());
}

void Importer::onDisconnected()
{
    delete m_iface;
    m_iface = 0;
    setIsRunning(false);
}

void Importer::callFinished(QDBusPendingCallWatcher* watcher)
{
    watcher->deleteLater();
    setIsRunning(false);
}

void Importer::import()
{
    if (!m_iface || !m_iface->isValid() || m_running) {
        return;
    }

    setIsRunning(true);
    
    QDBusPendingCall call = m_iface->asyncCall("ImportDict");
    QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(call, m_iface);
    connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)), SLOT(callFinished(QDBusPendingCallWatcher*)));
}

void Importer::clearDict(int type)
{
    if (!m_iface || !m_iface->isValid() || m_running) {
        return;
    }

    setIsRunning(true);

    QDBusPendingCall call = m_iface->asyncCall("ClearDict", type);
    QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(call, m_iface);
    connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)), SLOT(callFinished(QDBusPendingCallWatcher*)));
}

void Importer::setIsRunning(bool running)
{
    if (running != m_running) {
        m_running = running;
        if (m_running) {
            emit started();
        } else {
            emit finished();
        }
    }
}

