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

#ifndef SCELCONVERTER_H
#define SCELCONVERTER_H

#include <QObject>
#include <QProcess>
#include <QTemporaryFile>
#include <QMessageBox>

class ScelConverter : public QObject
{
    Q_OBJECT
public:
    explicit ScelConverter(QObject* parent = 0);
    void convert(const QString& from, const QString& to, bool removeOriginFile = true);

signals:
    void message(QMessageBox::Icon msgLevel, const QString& msg);
    void finished(bool succ);

public slots:
    void finished(int,QProcess::ExitStatus);
    void removeTempFile();

private:
    QProcess m_process;
    QTemporaryFile m_file;
    QString m_name;
    QString m_fromFile;
};



#endif // SCELCONVERTER_H
