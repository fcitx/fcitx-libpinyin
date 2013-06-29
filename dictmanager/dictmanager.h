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

#ifndef FCITX_DICTMANAGER_H
#define FCITX_DICTMANAGER_H

#include <QMainWindow>
#include "ui_dictmanager.h"

class ErrorOverlay;
class Importer;
class FileListModel;
class DictManager : public QMainWindow
{
    Q_OBJECT
public:
    explicit DictManager(QWidget* parent = 0);
    virtual ~DictManager();
public slots:
    void importFromFile();
    void importFromSogou();
    void importFromSogouOnline();
    void removeDict();
    void removeAllDict();
    void clearUserDict();
    void clearAllDict();
    void convertFinished(bool);
    void importerStarted();
    void importerFinished();

private:
    Ui::DictManager *m_ui;
    QAction* m_importFromFileAction;
    QAction* m_importFromSogou;
    QAction* m_importFromSogouOnline;
    FileListModel* m_model;
    Importer* m_importer;
    QAction* m_clearUserDictAction;
    QAction* m_clearAllDataAction;
    ErrorOverlay* m_errorOverlay;
};

#endif // FCITX_DICTMANAGER_H
