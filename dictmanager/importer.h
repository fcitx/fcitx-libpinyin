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

#ifndef FCITX_IMPORTER_H
#define FCITX_IMPORTER_H

#include <QString>
#include <fcitx-qt/fcitxqtconnection.h>

class QDBusInterface;
class QDBusPendingCallWatcher;
class Importer : public QObject {
    Q_OBJECT
    void realRun();
public:
    explicit Importer(QObject* parent = 0);
    virtual ~Importer();

    void import();
    void clearDict(int type);

signals:
    void started();
    void finished();

public slots:
    void callFinished(QDBusPendingCallWatcher*);

private slots:
    void onConnected();
    void onDisconnected();
    void setIsRunning(bool running);

private:
    FcitxQtConnection* m_connection;
    bool m_running;
    QDBusInterface* m_iface;
};


#endif // FCITX_IMPORTER_H
