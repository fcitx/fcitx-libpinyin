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

#ifndef FCITX_BROWSERDIALOG_H
#define FCITX_BROWSERDIALOG_H

#include <QDialog>
#include <QUrl>

namespace Ui {
class BrowserDialog;
}

class BrowserDialog : public QDialog
{
    Q_OBJECT
public:
    explicit BrowserDialog(QWidget* parent = 0);
    virtual ~BrowserDialog();

private:
    QString decodeName(const QByteArray& in);
    void download(const QUrl& url);
    Ui::BrowserDialog* m_ui;
    QString m_name;

public slots:
    void linkClicked(const QUrl& url);
    void showMessage(const QString& message);
    void downloadFinished(bool succ);
    void convertFinished(bool);
};

#endif // FCITX_BROWSERDIALOG_H
