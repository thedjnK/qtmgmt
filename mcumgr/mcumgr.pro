include(../qtmgmt-includes.pri)

QT += serialport $$ADDITIONAL_MODULES

TEMPLATE = lib

CONFIG += plugin
CONFIG += c++17
CONFIG += staticlib

INCLUDEPATH    += AuTerm/plugins/mcumgr
TARGET          = $$qtLibraryTarget(plugin_mcumgr)

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    AuTerm/AuTerm/AutEscape.cpp \
    AuTerm/plugins/mcumgr/crc16.cpp \
    AuTerm/plugins/mcumgr/smp_error.cpp \
    AuTerm/plugins/mcumgr/smp_group_enum_mgmt.cpp \
    AuTerm/plugins/mcumgr/smp_group_fs_mgmt.cpp \
    AuTerm/plugins/mcumgr/smp_group_os_mgmt.cpp \
    AuTerm/plugins/mcumgr/smp_group_img_mgmt.cpp \
    AuTerm/plugins/mcumgr/smp_group_settings_mgmt.cpp \
    AuTerm/plugins/mcumgr/smp_group_shell_mgmt.cpp \
    AuTerm/plugins/mcumgr/smp_group_stat_mgmt.cpp \
    AuTerm/plugins/mcumgr/smp_group_zephyr_mgmt.cpp \
    AuTerm/plugins/mcumgr/smp_message.cpp \
    AuTerm/plugins/mcumgr/smp_processor.cpp
#    AuTerm/plugins/mcumgr/smp_json.cpp \

HEADERS += \
    AuTerm/AuTerm/AutPlugin.h \
    AuTerm/AuTerm/AutEscape.h \
    AuTerm/plugins/mcumgr/crc16.h \
    AuTerm/plugins/mcumgr/smp_error.h \
    AuTerm/plugins/mcumgr/smp_group_array.h \
    AuTerm/plugins/mcumgr/smp_group_enum_mgmt.h \
    AuTerm/plugins/mcumgr/smp_group_fs_mgmt.h \
    AuTerm/plugins/mcumgr/smp_group_os_mgmt.h \
    AuTerm/plugins/mcumgr/smp_group_img_mgmt.h \
    AuTerm/plugins/mcumgr/smp_group_settings_mgmt.h \
    AuTerm/plugins/mcumgr/smp_group_shell_mgmt.h \
    AuTerm/plugins/mcumgr/smp_group_stat_mgmt.h \
    AuTerm/plugins/mcumgr/smp_group_zephyr_mgmt.h \
    AuTerm/plugins/mcumgr/smp_message.h \
    AuTerm/plugins/mcumgr/smp_processor.h \
    AuTerm/plugins/mcumgr/smp_transport.h \
    AuTerm/plugins/mcumgr/smp_group.h
#    AuTerm/plugins/mcumgr/smp_json.h \

DISTFILES += AuTerm/plugins/mcumgr/plugin_mcumgr.json

# Default rules for deployment.
unix {
    target.path = $$[QT_INSTALL_PLUGINS]/plugin_mcumgr
}
!isEmpty(target.path): INSTALLS += target

CONFIG += install_ok  # Do not cargo-cult this!

# Common build location
CONFIG(release, debug|release) {
    DESTDIR = ../release
} else {
    DESTDIR = ../debug
}

# Do not prefix with lib for non-static builds
!contains(CONFIG, static) {
    CONFIG += no_plugin_name_prefix
}

contains(DEFINES, PLUGIN_MCUMGR_TRANSPORT_UART) {
    SOURCES += \
	smp_uart.cpp

    HEADERS += \
	smp_uart.h
}

contains(DEFINES, PLUGIN_MCUMGR_TRANSPORT_BLUETOOTH) {
    SOURCES += \
	AuTerm/plugins/mcumgr/smp_bluetooth.cpp

    HEADERS += \
	AuTerm/plugins/mcumgr/smp_bluetooth.h
}

contains(DEFINES, PLUGIN_MCUMGR_TRANSPORT_UDP) {
    SOURCES += \
	AuTerm/plugins/mcumgr/smp_udp.cpp

    HEADERS += \
	AuTerm/plugins/mcumgr/smp_udp.h
}

contains(DEFINES, PLUGIN_MCUMGR_TRANSPORT_LORAWAN) {
    SOURCES += \
	AuTerm/plugins/mcumgr/smp_lorawan.cpp

    HEADERS += \
	AuTerm/plugins/mcumgr/smp_lorawan.h
}
