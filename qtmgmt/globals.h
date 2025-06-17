/******************************************************************************
** Copyright (C) 2025 Jamie M.
**
** Project: qtmgmt
**
** Module:  globals.h
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
#ifndef GLOBALS_H
#define GLOBALS_H

/******************************************************************************/
// Include Files
/******************************************************************************/
#include <QWaitCondition>
#include <QMutex>

/******************************************************************************/
// External Variable Declarations
/******************************************************************************/
extern QWaitCondition text_thread_wait_condition;
extern QMutex text_thread_mutex;

#endif // GLOBALS_H

/******************************************************************************/
// END OF FILE
/******************************************************************************/
