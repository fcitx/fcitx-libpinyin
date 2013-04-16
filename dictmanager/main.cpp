#include <QApplication>
#include <fcitx-qt/fcitxqtconnection.h>
#include "importer.h"
#include "dictmanager.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    DictManager win;
    win.show();

    return app.exec();
}
