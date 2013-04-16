#include <QString>
#include <fcitx-qt/fcitxqtconnection.h>

class Importer : public QObject {
    Q_OBJECT
    void realRun();
public:
    explicit Importer(QObject* parent = 0);
    virtual ~Importer();

    void run();
    void import();

signals:
    void finished();

private:
    FcitxQtConnection* m_connection;
    bool m_running;
public slots:
    void onConnected();
    void onDisconnected();
};
