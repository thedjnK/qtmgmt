/******************************************************************************
** Copyright (C) 2025 Jamie M.
**
** Project: qtmgmt
**
** Module:  text_thread.h
**
** Notes:
**
** License: This program is free software: you can redistribute it and/or
**          modify it under the terms of the GNU General Public License as
**          published by the Free Software Foundation, version 3.
**
**          This program is distributed in the hope that it will be useful,
**          but WITHOUT ANY WARRANTY; without even the implied warranty of
**          MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**          GNU General Public License for more details.
**
**          You should have received a copy of the GNU General Public License
**          along with this program.  If not, see http://www.gnu.org/licenses/
**
*******************************************************************************/
#ifndef TEXT_THREAD_H
#define TEXT_THREAD_H

/******************************************************************************/
// Include Files
/******************************************************************************/
#include <QObject>
#include <QThread>
#include <QTextStream>
#include <QMutexLocker>
#include <QDebug>
#include "globals.h"

/******************************************************************************/
// Class definitions
/******************************************************************************/
class text_thread : public QThread
{
    Q_OBJECT

public:
    text_thread(QObject *parent = nullptr);
    void set_quit();

signals:
    void data(QString data);

protected:
    void run() override;

private:
    bool should_quit;
};

#endif // TEXT_THREAD_H

/******************************************************************************/
// END OF FILE
/******************************************************************************/
