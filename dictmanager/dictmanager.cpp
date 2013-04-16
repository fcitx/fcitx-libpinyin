#include "dictmanager.h"
#include <libintl.h>
#include <QMenu>
#include <fcitx-utils/utils.h>
#include <QFileInfo>
#include <QMessageBox>

#define _(x) QString::fromUtf8(dgettext("fcitx-libpinyin", (x)))

DictManager::DictManager(QWidget* parent): QMainWindow(parent)
    ,m_ui(new Ui::DictManager)
{
    QMenu* menu = new QMenu(this);
    m_importFromFileAction = new QAction(_("From &File"), this);
    m_importFromSogou = new QAction(_("From &Sogou Cell"), this);
    m_importFromInputAction = new QAction(_("From &Input"), this);
    menu->addAction(m_importFromFileAction);
    menu->addAction(m_importFromSogou);
    menu->addAction(m_importFromInputAction);

    m_ui->setupUi(this);
    m_ui->importButton->setText(_("&Import"));
    m_ui->importButton->setMenu(menu);
    m_ui->removeButton->setText(_("&Remove"));
    m_ui->removeAllButton->setText(_("Remove &All"));

    connect(m_importFromFileAction, SIGNAL(triggered(bool)), SLOT(importFromFile(bool)));
    connect(m_importFromInputAction, SIGNAL(triggered(bool)), SLOT(importFromInput(bool)));
    connect(m_importFromSogou, SIGNAL(triggered(bool)), SLOT(importFromSogou(bool)));
}

DictManager::~DictManager()
{
    delete m_ui;
}

void DictManager::importFromFile(bool checked)
{
    

}

void DictManager::importFromInput(bool checked)
{

}

void DictManager::importFromSogou(bool checked)
{
    char* path = fcitx_utils_get_fcitx_path_with_filename("bindir", "scel2org");
    QFileInfo info(path);
    if (!info.isExecutable()) {
        QMessageBox::critical(this, _("Error"), _("Could not find scel2org, please check your install."));
        return;
    }

    free(path);
}

