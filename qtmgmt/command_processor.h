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
#include <smp_group_enum_mgmt.h>
#include <smp_group_fs_mgmt.h>
#include <smp_group_img_mgmt.h>
#include <smp_group_os_mgmt.h>
#include <smp_group_settings_mgmt.h>
#include <smp_group_shell_mgmt.h>
#include <smp_group_stat_mgmt.h>
#include <smp_group_zephyr_mgmt.h>
#include <smp_processor.h>
#if defined(PLUGIN_MCUMGR_TRANSPORT_UART)
#include <smp_uart.h>
#endif
#if defined(PLUGIN_MCUMGR_TRANSPORT_BLUETOOTH)
#include <smp_bluetooth.h>
#endif
#if defined(PLUGIN_MCUMGR_TRANSPORT_UDP)
#include <smp_udp.h>
#endif
#if defined(PLUGIN_MCUMGR_TRANSPORT_LORAWAN)
#include <smp_lorawan.h>
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

enum image_upload_mode_t {
    IMAGE_UPLOAD_MODE_NORMAL,
    IMAGE_UPLOAD_MODE_TEST,
    IMAGE_UPLOAD_MODE_CONFIRM,

    IMAGE_UPLOAD_MODE_COUNT
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
        QList<const QCommandLineOption *> option;
        bool required;
        bool exclusive;
    };

#if defined(PLUGIN_MCUMGR_TRANSPORT_UART)
    void add_transport_options_uart(QList<entry_t> *entries);
    int configure_transport_options_uart(smp_transport *transport, QCommandLineParser *parser);
#endif
#if defined(PLUGIN_MCUMGR_TRANSPORT_BLUETOOTH)
    void add_transport_options_bluetooth(QList<entry_t> *entries);
    int configure_transport_options_bluetooth(smp_transport *transport, QCommandLineParser *parser);
#endif
#if defined(PLUGIN_MCUMGR_TRANSPORT_UDP)
    void add_transport_options_udp(QList<entry_t> *entries);
    int configure_transport_options_udp(smp_transport *transport, QCommandLineParser *parser);
#endif
#if defined(PLUGIN_MCUMGR_TRANSPORT_LORAWAN)
    void add_transport_options_lorawan(QList<entry_t> *entries);
    int configure_transport_options_lorawan(smp_transport *transport, QCommandLineParser *parser);
#endif

    void add_group_options_enum(QList<entry_t> *entries, QString command);
    void add_group_options_fs(QList<entry_t> *entries, QString command);
    void add_group_options_os(QList<entry_t> *entries, QString command);
    void add_group_options_settings(QList<entry_t> *entries, QString command);
    void add_group_options_shell(QList<entry_t> *entries, QString command);
    void add_group_options_stat(QList<entry_t> *entries, QString command);
    void add_group_options_zephyr(QList<entry_t> *entries, QString command);
    void add_group_options_img(QList<entry_t> *entries, QString command);

    int run_group_enum(QCommandLineParser *parser, QString command);
    int run_group_fs(QCommandLineParser *parser, QString command);
    int run_group_os(QCommandLineParser *parser, QString command);
    int run_group_settings(QCommandLineParser *parser, QString command);
    int run_group_shell(QCommandLineParser *parser, QString command);
    int run_group_stat(QCommandLineParser *parser, QString command);
    int run_group_zephyr(QCommandLineParser *parser, QString command);
    int run_group_img(QCommandLineParser *parser, QString command);

    void set_group_transport_settings(smp_group *group);
    void set_group_transport_settings(smp_group *group, uint32_t timeout);

    void size_abbreviation(uint32_t size, QString *output);

    smp_processor *processor;

    smp_group_enum_mgmt *group_enum;
    smp_group_fs_mgmt *group_fs;
    smp_group_img_mgmt *group_img;
    smp_group_os_mgmt *group_os;
    smp_group_settings_mgmt *group_settings;
    smp_group_shell_mgmt *group_shell;
    smp_group_stat_mgmt *group_stat;
    smp_group_zephyr_mgmt *group_zephyr;

#if defined(PLUGIN_MCUMGR_TRANSPORT_UART)
    smp_uart *transport_uart;
#endif
#if defined(PLUGIN_MCUMGR_TRANSPORT_BLUETOOTH)
    smp_bluetooth *transport_bluetooth;
#endif
#if defined(PLUGIN_MCUMGR_TRANSPORT_UDP)
    smp_udp *transport_udp;
#endif
#if defined(PLUGIN_MCUMGR_TRANSPORT_LORAWAN)
    smp_lorawan *transport_lorawan;
#endif
    smp_transport *active_transport;
    smp_group *active_group;

    mcumgr_action_t mode;
    bool smp_v2;
    uint16_t smp_mtu;

    //Enumeration management
    uint16_t enum_mgmt_count;
    QList<uint16_t> *enum_mgmt_group_ids;
    uint16_t enum_mgmt_id;
    bool enum_mgmt_end;
    QList<enum_details_t> *enum_mgmt_group_details;
    enum_fields_present_t enum_mgmt_group_fields_present;

    //OS management
    uint32_t os_mgmt_mcumgr_parameters_buffer_size;
    uint32_t os_mgmt_mcumgr_parameters_buffer_count;
    QString *os_mgmt_os_application_info_response;
    QVariant *os_mgmt_bootloader_info_response;

    //Image management
    QList<image_state_t> *img_mgmt_get_state_images;
    QList<slot_info_t> *img_mgmt_slot_info_images;
    enum image_upload_mode_t upload_mode;
    QByteArray upload_hash;
    bool upload_reset;

    const QString value_transport_uart = "uart";
    const QString value_transport_bluetooth = "bluetooth";
    const QString value_transport_udp = "udp";
    const QString value_transport_lorawan = "lorawan";

    const uint16_t minimum_smp_mtu = 96;
    const uint16_t maximum_smp_mtu = 16384;

    const uint32_t default_transport_uart_baud = 115200;
    const uint16_t default_transport_udp_port = 1337;
    const uint16_t default_transport_lorawan_port = 1883;
    const uint16_t default_transport_lorawan_port_ssl = 8883;

    typedef void (command_processor::*add_group_options_t)(QList<entry_t> *entries, QString command);
    typedef int (command_processor::*run_group_t)(QCommandLineParser *parser, QString command);
    typedef void (command_processor::*add_transport_options_t)(QList<entry_t> *entries);
    typedef int (command_processor::*configure_transport_options_t)(smp_transport *transport, QCommandLineParser *parser);

    struct supported_command_t {
        QString name;
        QStringList arguments;
    };

    struct supported_group_t {
        QString name;
        QStringList arguments;
        uint16_t group_id;
        smp_group *group;
        add_group_options_t options_function;
        run_group_t run_function;
        QList<supported_command_t> commands;
    };

    struct supported_transport_t {
        QString name;
        QStringList arguments;
        smp_transport *transport;
        add_transport_options_t options_function;
        configure_transport_options_t configure_function;
    };

    const QList<supported_group_t> supported_groups = {
#if 0
        {"Enumeration management", {"enumeration", "enum"}, SMP_GROUP_ID_ENUM, group_enum, &command_processor::add_group_options_enum, &command_processor::run_group_enum,
            {
                {"Number of supported groups", {"count"}},
                {"List supported groups", {"list"}},
                {"Get information on one supported group", {"single"}},
                {"Get details of supported groups", {"details"}}
            }
        },
        {"Filesystem management", {"filesystem", "fs"}, SMP_GROUP_ID_FS, group_fs, &command_processor::add_group_options_fs, &command_processor::run_group_fs,
            {
                {"Upload file to device", {"upload"}},
                {"Download file from device", {"download"}},
                {"Get file status", {"status"}},
                {"Get hash/checksum of file", {"hash", "checksum", "hash-checksum"}},
                {"Get a list of supported hashes/checksums", {"supported-hashes", "supported-checksums", "supported-hashes-checksums"}},
                {"Close open file", {"close"}}
            }
        },
#endif
        {"Image management", {"image", "img"}, SMP_GROUP_ID_IMG, group_img, &command_processor::add_group_options_img, &command_processor::run_group_img,
            {
               {"Get image state(s)", {"get-state"}},
               {"Set image state", {"set-state"}},
               {"Upload firmware update", {"upload"}},
               {"Erase slot", {"erase"}},
               {"Get information on slots", {"slot-info"}}
            }
        },
        {"Operating system management", {"os"}, SMP_GROUP_ID_OS, group_os, &command_processor::add_group_options_os, &command_processor::run_group_os,
             {
                {"Echo text back", {"echo"}},
#if 0
                {"List running tasks/threads", {"tasks"}},
                {"Get memory pool details", {"memory"}},
#endif
                {"Reset device", {"reset"}},
#if 0
                {"Get supported MCUmgr parameters", {"mcumgr-parameters"}},
                {"Get application information", {"application-info"}},
                {"Get the device time and date", {"get-date-time"}},
                {"Set the device time and date", {"set-date-time"}},
                {"Get information on bootloader", {"bootloader-info"}}
#endif
            }
        }
#if 0
        {"Settings management", {"settings"}, SMP_GROUP_ID_SETTINGS, group_settings, &command_processor::add_group_options_settings}, &command_processor::run_group_settings,
        {"Shell management", {"shell"}, SMP_GROUP_ID_SHELL, group_shell, &command_processor::add_group_options_shell}, &command_processor::run_group_shell,
        {"Statistics management", {"statistics", "stats"}, SMP_GROUP_ID_STATS, group_stat, &command_processor::add_group_options_stat}, &command_processor::run_group_stat,
        {"Zephyr basic management", {"zephyr"}, SMP_GROUP_ID_ZEPHYR, group_zephyr, &command_processor::add_group_options_zephyr}, &command_processor::run_group_zephyr,
#endif
    };

    const QList<supported_transport_t> supported_transports = {
#if defined(PLUGIN_MCUMGR_TRANSPORT_UART)
        {"UART transport", {"uart", "serial"}, transport_uart, &command_processor::add_transport_options_uart, &command_processor::configure_transport_options_uart},
#endif
#if defined(PLUGIN_MCUMGR_TRANSPORT_BLUETOOTH)
        {"Bluetooth Low Energy transport", {"bluetooth", "bt"}, transport_bluetooth, &command_processor::add_transport_options_bluetooth, &command_processor::configure_transport_options_bluetooth},
#endif
#if defined(PLUGIN_MCUMGR_TRANSPORT_UDP)
        {"UDP transport", {"udp"}, transport_udp, &command_processor::add_transport_options_udp, &command_processor::configure_transport_options_udp},
#endif
#if defined(PLUGIN_MCUMGR_TRANSPORT_LORAWAN)
        {"LoRaWAN (TTS/MQTT) transport", {"lorawan"}, transport_lorawan, &command_processor::add_transport_options_lorawan, &command_processor::configure_transport_options_lorawan},
#endif
    };
};

#endif // COMMAND_PROCESSOR_H

/******************************************************************************/
// END OF FILE
/******************************************************************************/
