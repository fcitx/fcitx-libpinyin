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

#include "dictmanager.h"
#include "filelistmodel.h"
#include "importer.h"
#include "browserdialog.h"
#include <libintl.h>
#include <QMenu>
#include <QInputDialog>
#include <fcitx-utils/utils.h>
#include <QFileInfo>
#include <QMessageBox>
#include <QProcess>
#include <QDebug>
#include <fcitx-config/xdg.h>
#include <QFileDialog>
#include "guicommon.h"
#include "scelconverter.h"

DictManager::DictManager(QWidget* parent): QMainWindow(parent)
    ,m_ui(new Ui::DictManager)
    ,m_importer(new Importer(this))
{
    m_ui->setupUi(this);
    setWindowIcon(QIcon::fromTheme("accessories-dictionary"));
    setWindowTitle(_("Manage Pinyin dictionary"));
    QMenu* menu = new QMenu(this);
    m_importFromFileAction = new QAction(_("From &File"), this);
    m_importFromSogou = new QAction(_("From &Sogou Cell"), this);
    m_importFromSogouOnline = new QAction(_("Browse &Sogou Cell Online"), this);
    menu->addAction(m_importFromFileAction);
    menu->addAction(m_importFromSogou);
    menu->addAction(m_importFromSogouOnline);
    m_ui->importButton->setMenu(menu);

    menu = new QMenu(this);
    m_clearUserDictAction = new QAction(_("&Clear User Data"), this);
    m_clearAllDataAction = new QAction(_("&Clear All Data"), this);
    menu->addAction(m_clearUserDictAction);
    menu->addAction(m_clearAllDataAction);
    m_ui->clearDictButton->setMenu(menu);

    m_model = new FileListModel(this);

    m_ui->importButton->setText(_("&Import"));
    m_ui->removeButton->setText(_("&Remove"));
    m_ui->removeAllButton->setText(_("Remove &All"));
    m_ui->clearDictButton->setText(_("&Clear Dict"));
    m_ui->listView->setModel(m_model);

    connect(m_importFromFileAction, SIGNAL(triggered(bool)), SLOT(importFromFile()));
    connect(m_importFromSogou, SIGNAL(triggered(bool)), SLOT(importFromSogou()));
    connect(m_importFromSogouOnline, SIGNAL(triggered(bool)), SLOT(importFromSogouOnline()));
    connect(m_clearUserDictAction, SIGNAL(triggered(bool)), SLOT(clearUserDict()));
    connect(m_clearAllDataAction, SIGNAL(triggered(bool)), SLOT(clearAllDict()));

    connect(m_ui->removeButton, SIGNAL(clicked(bool)), SLOT(removeDict()));
    connect(m_ui->removeAllButton, SIGNAL(clicked(bool)), SLOT(removeAllDict()));

}

DictManager::~DictManager()
{
    delete m_ui;
}

void DictManager::importFromFile()
{
    QString name = QFileDialog::getOpenFileName(this, _("Select dictionary file"));
    if (name.isEmpty()) {
        return;
    }

    QFileInfo info(name);
    QString importName = info.fileName();

    if (importName.endsWith(".txt")) {
        importName = importName.left(importName.size() - 4);
    }

    bool ok;
    importName = QInputDialog::getText(this, _("Dict name"), _("Dictionary Name"), QLineEdit::Normal, importName, &ok);
    if (!ok || importName.isEmpty()) {
        return;
    }

    char* fullname;
    FcitxXDGGetFileUserWithPrefix(m_model->dictDir().toLocal8Bit(), importName.append(".txt").toLocal8Bit(), NULL, &fullname);
    if (!QFile::copy(name, QString::fromLocal8Bit(fullname))) {
        QMessageBox::warning(this, _("Copy file failed"), _("Copy file failed. Please check you have permission or disk is not full."));
    } else {
        m_importer->run();
        m_model->loadFileList();
    }

    free(fullname);
}

void DictManager::importFromSogou()
{
    QString name = QFileDialog::getOpenFileName(this,
                                                _("Select scel file"),
                                               QString(),
                                               _("Scel file (*.scel)")
                                               );
    if (name.isEmpty()) {
        return;
    }


    QFileInfo info(name);
    QString importName = info.fileName();
    if (importName.endsWith(".scel")) {
        importName = importName.left(importName.size() - 5);
    }

    bool ok;
    importName = QInputDialog::getText(this, _("Dict name"), _("Dictionary Name"), QLineEdit::Normal, importName, &ok);
    if (!ok || importName.isEmpty()) {
        return;
    }
    char* fullname;
    FcitxXDGGetFileUserWithPrefix(m_model->dictDir().toLocal8Bit(), importName.append(".txt").toLocal8Bit(), NULL, &fullname);
    ScelConverter *converter = new ScelConverter;
    connect(converter, SIGNAL(finished(bool)), SLOT(convertFinished(bool)));
    setEnabled(false);

    converter->convert(name, QString::fromLocal8Bit(fullname), false);
}

void DictManager::convertFinished(bool succ)
{
    setEnabled(true);
    if (!succ) {
        QMessageBox::warning(this, "Convertion failed", "Convert failed, please check this file is valid scel file or not.");
    } else {
        m_importer->run();
        m_model->loadFileList();
    }
}

void DictManager::importFromSogouOnline()
{
    BrowserDialog dialog(this);
    int result = dialog.exec();
    if (result == QDialog::Accepted) {
        m_importer->run();
        m_model->loadFileList();
    }
}

void DictManager::removeDict()
{
    QModelIndex index = m_ui->listView->currentIndex();
    if (!index.isValid()) {
        return;
    }

    QString curName = index.data(Qt::DisplayRole).toString();
    QString fileName = index.data(Qt::UserRole).toString();
    char* fullname = NULL;
    FcitxXDGGetFileUserWithPrefix(NULL, fileName.toLocal8Bit(), NULL, &fullname);

    int ret = QMessageBox::question(this,
                                    _("Confirm deletion"),
                                    _("Are you sure to delete %1?").arg(curName),
                                    QMessageBox::Ok | QMessageBox::Cancel);

    if (ret == QMessageBox::Ok) {
        bool ok = QFile::remove(QString::fromLocal8Bit(fullname));
        if (!ok) {
            QMessageBox::warning(this,
                                 _("File Operation Failed"),
                                 _("Error while deleting %1.").arg(curName)
                                );
        } else {
            m_importer->run();
            m_model->loadFileList();
        }
    }
    free(fullname);
}

void DictManager::removeAllDict()
{

    for (int i = 0; i < m_model->rowCount(); i ++) {
        QModelIndex index = m_model->index(i, 0);

        QString fileName = index.data(Qt::UserRole).toString();
        char* fullname = NULL;
        FcitxXDGGetFileUserWithPrefix(NULL, fileName.toLocal8Bit(), NULL, &fullname);
        QFile::remove(QString::fromLocal8Bit(fullname));
        free(fullname);
    }

    m_importer->clearDict(1);
    m_model->loadFileList();
}

void DictManager::clearUserDict()
{
    m_importer->clearDict(0);
}

void DictManager::clearAllDict()
{
    m_importer->clearDict(2);
}

