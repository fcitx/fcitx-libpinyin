#ifndef FCITX_DICTMANAGER_H
#define FCITX_DICTMANAGER_H

#include <QMainWindow>
#include "ui_dictmanager.h"

class DictManager : public QMainWindow
{
    Q_OBJECT
public:
    explicit DictManager(QWidget* parent = 0);
    virtual ~DictManager();

private:
    Ui::DictManager *m_ui;
    QAction* m_importFromFileAction;
    QAction* m_importFromSogou;
    QAction* m_importFromInputAction;
public slots:
    void importFromFile(bool checked);
    void importFromInput(bool checked);
    void importFromSogou(bool checked);
};

#endif // FCITX_DICTMANAGER_H
