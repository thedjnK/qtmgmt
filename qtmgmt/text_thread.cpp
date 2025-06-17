/******************************************************************************
** Copyright (C) 2025 Jamie M.
**
** Project: qtmgmt
**
** Module:  text_thread.cpp
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

/******************************************************************************/
// Include Files
/******************************************************************************/
#include "text_thread.h"

/******************************************************************************/
// Local Functions or Private Members
/******************************************************************************/
text_thread::text_thread(QObject *parent) : QThread(parent)
{
    should_quit = false;
}

void text_thread::run()
{
    QTextStream *stdin_text_stream = new QTextStream(stdin);
    const QMutexLocker mutex_locker(&text_thread_mutex);

    while (1)
    {
        if (should_quit == true)
        {
            delete stdin_text_stream;
            return;
        }

        emit data(stdin_text_stream->readLine());

        text_thread_wait_condition.wait(&text_thread_mutex);
    }
}

void text_thread::set_quit()
{
    should_quit = true;
}

/******************************************************************************/
// END OF FILE
/******************************************************************************/
