/******************************************************************************
** Copyright (C) 2024 Jamie M.
**
** Project: qtmgmt
**
** Module:  command_processor.h
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
#ifndef COMMAND_PROCESSOR_H
#define COMMAND_PROCESSOR_H

/******************************************************************************/
// Include Files
/******************************************************************************/
#include <QObject>
#include <QTimer>
#include <QCommandLineParser>
#include <smp_group_os_mgmt.h>
#include <smp_processor.h>
#if defined(PLUGIN_MCUMGR_TRANSPORT_UART)
#include <smp_uart.h>
#endif
#if defined(PLUGIN_MCUMGR_TRANSPORT_UDP)
#include <smp_udp.h>
#endif
#if defined(PLUGIN_MCUMGR_TRANSPORT_BLUETOOTH)
#include <smp_bluetooth.h>
#endif

/******************************************************************************/
// Enum typedefs
/******************************************************************************/
enum mcumgr_action_t {
    ACTION_IDLE,

    ACTION_IMG_UPLOAD,
    ACTION_IMG_UPLOAD_SET,
    ACTION_OS_UPLOAD_RESET,
    ACTION_IMG_IMAGE_LIST,
    ACTION_IMG_IMAGE_SET,
    ACTION_IMG_IMAGE_ERASE,
    ACTION_IMG_IMAGE_SLOT_INFO,

    ACTION_OS_ECHO,
    ACTION_OS_TASK_STATS,
    ACTION_OS_MEMORY_POOL,
    ACTION_OS_RESET,
    ACTION_OS_DATETIME_GET,
    ACTION_OS_DATETIME_SET,
    ACTION_OS_MCUMGR_BUFFER,
    ACTION_OS_OS_APPLICATION_INFO,
    ACTION_OS_BOOTLOADER_INFO,

    ACTION_SHELL_EXECUTE,

    ACTION_STAT_GROUP_DATA,
    ACTION_STAT_LIST_GROUPS,

    ACTION_FS_UPLOAD,
    ACTION_FS_DOWNLOAD,
    ACTION_FS_STATUS,
    ACTION_FS_HASH_CHECKSUM,
    ACTION_FS_SUPPORTED_HASHES_CHECKSUMS,

    ACTION_SETTINGS_READ,
    ACTION_SETTINGS_WRITE,
    ACTION_SETTINGS_DELETE,
    ACTION_SETTINGS_COMMIT,
    ACTION_SETTINGS_LOAD,
    ACTION_SETTINGS_SAVE,

    ACTION_ZEPHYR_STORAGE_ERASE,

    ACTION_ENUM_COUNT,
    ACTION_ENUM_LIST,
    ACTION_ENUM_SINGLE,
    ACTION_ENUM_DETAILS,

    ACTION_CUSTOM,
};

enum exit_code_t {
    EXIT_CODE_SUCCESS = 0,
    EXIT_CODE_MISSING_REQUIRED_ARGUMENTS = -1,
    EXIT_CODE_UNKNOWN_ARGUMENTS_PROVIDED = -2,
    EXIT_CODE_INVALID_TRANSPORT = -3,
    EXIT_CODE_INVALID_GROUP = -4,
    EXIT_CODE_NUMERIAL_ARGUMENT_CONVERSION_FAILED = -5,
    EXIT_CODE_NUMERIAL_ARGUMENT_OUT_OF_RANGE = -6,
    EXIT_CODE_ARGUMENT_VALUE_NOT_VALID = -7,
    EXIT_CODE_TRANSPORT_OPEN_FAILED = -8,
};

/******************************************************************************/
// Class definitions
/******************************************************************************/
class command_processor : public QObject
{
    Q_OBJECT

public:
    explicit command_processor(QObject *parent = nullptr);
    ~command_processor();

private slots:
    void run();
    //void custom_message_callback(enum custom_message_callback_t type, smp_error_t *data);
    void status(uint8_t user_data, group_status status, QString error_string);
    void progress(uint8_t user_data, uint8_t percent);
    void transport_connected();
    void transport_disconnected();

signals:

private:
    struct entry_t {
        const QCommandLineOption *option;
        bool required;
        bool exclusive;
        const QCommandLineOption *exclusive_option;
    };

#if defined(PLUGIN_MCUMGR_TRANSPORT_UART)
    void add_transport_options_uart(QList<entry_t> *entries);
    int configure_transport_options_uart(smp_uart *transport, QCommandLineParser *parser);
#endif
#if defined(PLUGIN_MCUMGR_TRANSPORT_BLUETOOTH)
    void add_transport_options_bluetooth(QList<entry_t> *entries);
    int configure_transport_options_bluetooth(smp_bluetooth *transport, QCommandLineParser *parser);
#endif
#if defined(PLUGIN_MCUMGR_TRANSPORT_UDP)
    void add_transport_options_udp(QList<entry_t> *entries);
    int configure_transport_options_udp(smp_udp *transport, QCommandLineParser *parser);
#endif
#if defined(PLUGIN_MCUMGR_TRANSPORT_LORAWAN)
    void add_transport_options_lorawan(QList<entry_t> *entries);
#endif

    void add_group_options_enum(QList<entry_t> *entries, QString command);
    void add_group_options_fs(QList<entry_t> *entries, QString command);
    void add_group_options_os(QList<entry_t> *entries, QString command);
    void add_group_options_settings(QList<entry_t> *entries, QString command);
    void add_group_options_shell(QList<entry_t> *entries, QString command);
    void add_group_options_stat(QList<entry_t> *entries, QString command);
    void add_group_options_zephyr(QList<entry_t> *entries, QString command);
    void add_group_options_img(QList<entry_t> *entries, QString command);

    //int run_group_enum(QCommandLineParser *parser, QString command);
    //int run_group_fs(QCommandLineParser *parser, QString command);
    int run_group_os(QCommandLineParser *parser, QString command);
    //int run_group_settings(QCommandLineParser *parser, QString command);
    //int run_group_shell(QCommandLineParser *parser, QString command);
    //int run_group_stat(QCommandLineParser *parser, QString command);
    //int run_group_zephyr(QCommandLineParser *parser, QString command);
    //int run_group_img(QCommandLineParser *parser, QString command);

    void set_group_transport_settings(smp_group *group);
    void set_group_transport_settings(smp_group *group, uint32_t timeout);

    smp_processor *processor;
    smp_group_os_mgmt *group_os;
#if defined(PLUGIN_MCUMGR_TRANSPORT_UART)
    smp_uart *transport_uart;
#endif
#if defined(PLUGIN_MCUMGR_TRANSPORT_UDP)
    smp_udp *transport_udp;
#endif
#if defined(PLUGIN_MCUMGR_TRANSPORT_BLUETOOTH)
    smp_bluetooth *transport_bluetooth;
#endif
    smp_transport *active_transport;
    smp_group *active_group;

    mcumgr_action_t mode;
    bool smp_v2;
    uint16_t smp_mtu;

    const QString value_transport_uart = "uart";
    const QString value_transport_bluetooth = "bluetooth";
    const QString value_transport_udp = "udp";
    const QString value_transport_lorawan = "lorawan";
};

#endif // COMMAND_PROCESSOR_H

/******************************************************************************/
// END OF FILE
/******************************************************************************/
