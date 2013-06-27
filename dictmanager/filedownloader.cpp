#include "filedownloader.h"
#include "guicommon.h"
#include <QTemporaryFile>

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
