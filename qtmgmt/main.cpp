/******************************************************************************
** Copyright (C) 2024 Jamie M.
**
** Project: qtmgmt
**
** Module:  main.cpp
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
#include <QCoreApplication>
#include <QLocale>
#include <QTranslator>
#include "command_processor.h"

/******************************************************************************/
// Global Functions or Non Class Members
/******************************************************************************/
int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    command_processor *app_processor = new command_processor(a.instance());
    int exit_code;

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "qtmgmt_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }

    //Run command processor function until exit
    exit_code = a.exec();
    delete app_processor;
    app_processor = nullptr;
    return exit_code;
}

/******************************************************************************/
// END OF FILE
/******************************************************************************/
