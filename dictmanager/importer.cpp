#include <QFile>

#include "importer.h"
#include "common.h"
#include <pinyin.h>
#include <QDBusInterface>
#include <QDBusPendingCall>
#include <QDebug>
#include <fcitx-config/xdg.h>
#include <fcitx-utils/utf8.h>

using namespace pinyin;

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

        qDebug() << "AAAAAAAAAAA";
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

