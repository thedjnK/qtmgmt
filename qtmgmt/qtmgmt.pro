include(../qtmgmt-includes.pri)

QT = core
QT += serialport $$ADDITIONAL_MODULES

CONFIG += c++17 cmdline

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

INCLUDEPATH    += ../mcumgr
INCLUDEPATH    += ../mcumgr/AuTerm/plugins/mcumgr

SOURCES += \
	command_processor.cpp \
	main.cpp

HEADERS += \
    ../mcumgr/AuTerm/AuTerm/AutPlugin.h \
    ../mcumgr/AuTerm/AuTerm/AutEscape.h \
    ../mcumgr/AuTerm/plugins/mcumgr/crc16.h \
    ../mcumgr/AuTerm/plugins/mcumgr/smp_error.h \
    ../mcumgr/AuTerm/plugins/mcumgr/smp_group_array.h \
    ../mcumgr/AuTerm/plugins/mcumgr/smp_group_enum_mgmt.h \
    ../mcumgr/AuTerm/plugins/mcumgr/smp_group_fs_mgmt.h \
    ../mcumgr/AuTerm/plugins/mcumgr/smp_group_os_mgmt.h \
    ../mcumgr/AuTerm/plugins/mcumgr/smp_group_img_mgmt.h \
    ../mcumgr/AuTerm/plugins/mcumgr/smp_group_settings_mgmt.h \
    ../mcumgr/AuTerm/plugins/mcumgr/smp_group_shell_mgmt.h \
    ../mcumgr/AuTerm/plugins/mcumgr/smp_group_stat_mgmt.h \
    ../mcumgr/AuTerm/plugins/mcumgr/smp_group_zephyr_mgmt.h \
    ../mcumgr/AuTerm/plugins/mcumgr/smp_message.h \
    ../mcumgr/AuTerm/plugins/mcumgr/smp_processor.h \
    ../mcumgr/AuTerm/plugins/mcumgr/smp_transport.h \
    ../mcumgr/AuTerm/plugins/mcumgr/smp_group.h \
    ../mcumgr/smp_uart.h \
    command_processor.h \
    qtmgmt.h

#    ../mcumgr/AuTerm/plugins/mcumgr/smp_json.h \

TRANSLATIONS += \
    qtmgmt_en_GB.ts

CONFIG += lrelease
CONFIG += embed_translations

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

# Common build location
CONFIG(release, debug|release) {
    DESTDIR = ../release
} else {
    DESTDIR = ../debug
}

contains(CONFIG, static) {
    win32: LIBS += -L$$DESTDIR -lplugin_mcumgr
    else: LIBS += -L$$DESTDIR -lplugin_mcumgr

    win32-g++: PRE_TARGETDEPS += $$DESTDIR/libplugin_mcumgr.a
    else:win32:!win32-g++: PRE_TARGETDEPS += $$DESTDIR/plugin_mcumgr.lib
    else: PRE_TARGETDEPS += $$DESTDIR/libplugin_mcumgr.a
} else {
    win32: LIBS += -L$$DESTDIR -lplugin_mcumgr.dll
    else: LIBS += -L$$DESTDIR -l:plugin_mcumgr.so

    win32-g++: PRE_TARGETDEPS += $$DESTDIR/plugin_mcumgr.dll
    else: PRE_TARGETDEPS += $$DESTDIR/plugin_mcumgr.so
}
