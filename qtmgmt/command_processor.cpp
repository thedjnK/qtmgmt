/******************************************************************************
** Copyright (C) 2024 Jamie M.
**
** Project: qtmgmt
**
** Module:  command_processor.cpp
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
#include "command_processor.h"
#include <QDebug>
#include <AuTerm/AuTerm/AutEscape.h>

//UART
const QCommandLineOption option_transport_uart_port("port", "UART port", "port");
const QCommandLineOption option_transport_uart_baud("baud", "UART baud rate (default: 115200)", "port");
const QCommandLineOption option_transport_uart_flow_control("flow-control", "UART flow control (default: none, can be: none, hardware, software)", "flow-control");
const QCommandLineOption option_transport_uart_parity("parity", "UART parity (default: none, can be: none, even, odd, space, mark)", "parity");
const QCommandLineOption option_transport_uart_data_bits("data-bits", "UART data bits (default: 8, can be: 7, 8)", "data-bits");
const QCommandLineOption option_transport_uart_stop_bits("stop-bits", "UART stop bits (default: 1, can be: 1, 1.5, 2)", "stop-bits");

//Bluetooth
const QCommandLineOption option_transport_bluetooth_name("name", "Bluetooth device name", "name");
const QCommandLineOption option_transport_bluetooth_address("address", "Bluetooth device address", "address");

//UDP
const QCommandLineOption option_transport_udp_host("host", "UDP host", "host");
const QCommandLineOption option_transport_udp_port("port", "UDP port", "port");

//LoRaWAN
const QCommandLineOption option_transport_lorawan_host("host", "LoRaWAN host", "host");
const QCommandLineOption option_transport_lorawan_port("port", "LoRaWAN port", "port");
const QCommandLineOption option_transport_lorawan_tls("tls", "LoRaWAN use TLS for connection");
const QCommandLineOption option_transport_lorawan_no_tls("no-tls", "LoRaWAN do not use TLS for connection");
const QCommandLineOption option_transport_lorawan_username("username", "LoRaWAN username", "username");
const QCommandLineOption option_transport_lorawan_password("password", "LoRaWAN password", "password");
const QCommandLineOption option_transport_lorawan_topic("topic", "LoRaWAN MQTT topic", "topic");
const QCommandLineOption option_transport_lorawan_frame_port("frame-port", "LoRaWAN frame port", "port");

const QString value_group_enum = "enum";
const QString value_group_fs = "fs";
const QString value_group_os = "os";
const QString value_group_settings = "settings";
const QString value_group_shell = "shell";
const QString value_group_stat = "stat";
const QString value_group_zephyr = "zephyr";
const QString value_group_img = "img";

//const QString value_command_img_ = "";
const QString value_command_enum_count = "count";
const QString value_command_enum_list = "list";
const QString value_command_enum_single = "single";
const QString value_command_enum_details = "details";

const QString value_command_fs_upload = "upload";
const QString value_command_fs_download = "download";
const QString value_command_fs_status = "status";
const QStringList value_command_fs_hash_checksum = QStringList() << "hash" << "checksum" << "hash-checksum";
const QStringList value_command_fs_supported_hashes_checksums = QStringList() << "supported-hashes" << "supported-checksums" << "supported-hashes-checksums";
const QString value_command_fs_close = "close";

const QString value_command_os_echo = "echo";
const QString value_command_os_tasks = "tasks";
const QString value_command_os_memory = "memory";
const QString value_command_os_reset = "reset";
const QString value_command_os_mcumgr_parameters = "mcumgr-parameters";
const QString value_command_os_application_info = "application-info";
const QString value_command_os_get_date_time = "get-date-time";
const QString value_command_os_set_date_time = "set-date-time";
const QString value_command_os_bootloader_info = "bootloader-info";

const QString value_command_img_get_state = "get-state";
const QString value_command_img_set_state = "set-state";
const QString value_command_img_upload = "upload";
const QString value_command_img_erase = "erase";
const QString value_command_img_slot_info = "slot-info";

//const QString value_command_settings_ = "";
//const QString value_command_shell_ = "";
//const QString value_command_stat_ = "";
//const QString value_command_zephyr_ = "";

//Enumeration management group
const QCommandLineOption option_command_enum_index("index", "Index (0-based)", "index");
const QCommandLineOption option_command_enum_groups("groups", "List of groups (comma separated)", "groups");

//Filesystem management group
const QCommandLineOption option_command_fs_local_file("local-file", "Local PC file", "filename");
const QCommandLineOption option_command_fs_remote_file("remote-file", "Remote Zephyr file", "filename");
const QCommandLineOption option_command_fs_hash_checksum(QStringList() << "hash" << "checksum", "Hash/checksum", "type");

//OS management group
const QCommandLineOption option_command_os_data("data", "Text data", "data");
const QCommandLineOption option_command_os_force("force", "Force resetting device even if busy");
const QCommandLineOption option_command_os_format("format", "Info format string", "format");
const QCommandLineOption option_command_os_datetime("datetime", "Date and time", "TODO");
const QCommandLineOption option_command_os_query("query", "Query string", "query");

//Image management group
const QCommandLineOption option_command_img_hash("hash", "Hash of image", "hash");
const QCommandLineOption option_command_img_confirm("confirm", "Mark as confirmed instead of test");
const QCommandLineOption option_command_img_image("image", "Image number", "image");
const QCommandLineOption option_command_img_file("file", "Firmware update", "file");
const QCommandLineOption option_command_img_upgrade("upgrade", "Only accept upgrades");
const QCommandLineOption option_command_img_slot("slot", "Slot number", "slot");

//SMP options
const QCommandLineOption option_mtu("mtu", "MTU (default: 256, can be: 96-16384)", "mtu");
const QCommandLineOption option_smp_v1("smp-v1", "Use SMP version 1");
const QCommandLineOption option_smp_v2("smp-v2", "Use SMP version 2 (default)");

/******************************************************************************/
// Local Functions or Private Members
/******************************************************************************/
command_processor::command_processor(QObject *parent) : QObject{parent}
{
    processor = nullptr;
    group_enum = nullptr;
    group_fs = nullptr;
    group_img = nullptr;
    group_os = nullptr;
    group_settings = nullptr;
    group_shell = nullptr;
    group_stat = nullptr;
    group_zephyr = nullptr;
    transport_uart = nullptr;
    active_transport = nullptr;
    mode = ACTION_IDLE;
    smp_v2 = true;
    smp_mtu = 256;
    img_mgmt_get_state_images = nullptr;

    //Execute run function in event loop so that QCoreApplication::exit() works
    QTimer::singleShot(0, this, SLOT(run()));
}

command_processor::~command_processor()
{
    if (transport_uart != nullptr)
    {
        if (transport_uart->is_connected() == 1)
        {
            transport_uart->disconnect(true);
        }

        delete transport_uart;
        transport_uart = nullptr;
    }

    if (img_mgmt_get_state_images != nullptr)
    {
        delete img_mgmt_get_state_images;
        img_mgmt_get_state_images = nullptr;
    }

    if (active_transport != nullptr)
    {
        active_transport = nullptr;
    }

    if (processor != nullptr)
    {
        delete processor;
        processor = nullptr;
    }

    if (group_enum != nullptr)
    {
        delete group_enum;
        group_enum = nullptr;
    }

    if (group_fs != nullptr)
    {
        delete group_fs;
        group_fs = nullptr;
    }

    if (group_img != nullptr)
    {
        delete group_img;
        group_img = nullptr;
    }

    if (group_os != nullptr)
    {
        delete group_os;
        group_os = nullptr;
    }

    if (group_settings != nullptr)
    {
        delete group_settings;
        group_settings = nullptr;
    }

    if (group_shell != nullptr)
    {
        delete group_shell;
        group_shell = nullptr;
    }

    if (group_stat != nullptr)
    {
        delete group_stat;
        group_stat = nullptr;
    }

    if (group_zephyr != nullptr)
    {
        delete group_zephyr;
        group_zephyr = nullptr;
    }
}

void command_processor::run()
{
    QCommandLineParser parser;
    QList<entry_t> entries;
    const QCommandLineOption option_transport("transport", "MCUmgr transport", "type");
    const QCommandLineOption option_group("group", "MCUmgr group", "type");
    const QCommandLineOption option_command("command", "MCUmgr group command", "command");
    const QCommandLineOption option_help(QStringList()
#ifdef Q_OS_WIN
                                             << "?"
#endif
                                             << "h" << "help", "Show help/command line options.");
    const QCommandLineOption option_version = parser.addVersionOption();
    int exit_code = EXIT_CODE_SUCCESS;
    QString user_transport;
    QString user_group;
    uint8_t i;
    uint8_t l;
    bool failed = false;
    QEventLoop wait_loop;

    parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
    parser.addOption(option_help);
    parser.addOption(option_transport);
    parser.addOption(option_group);
    parser.addOption(option_command);
    parser.addOption(option_mtu);
    parser.parse(QCoreApplication::arguments());

    if (parser.isSet(option_version))
    {
        fputs(qPrintable(QCoreApplication::applicationName() % tr(" version ") % QCoreApplication::applicationVersion() % "\n"), stdout);
        QCoreApplication::exit(EXIT_CODE_SUCCESS);
        return;
    }

    if (!parser.isSet(option_transport) || !parser.isSet(option_group) || !parser.isSet(option_command))
    {
        if (!parser.isSet(option_transport))
        {
            fputs(qPrintable(tr("Missing required argument: ") % "--" % option_transport.names().join(" or --") % "\n"), stdout);
        }

        if (!parser.isSet(option_group))
        {
            fputs(qPrintable(tr("Missing required argument: ") % "--" % option_group.names().join(" or --") % "\n"), stdout);
        }

        if (!parser.isSet(option_command))
        {
            fputs(qPrintable(tr("Missing required argument: ") % "--" % option_command.names().join(" or --") % "\n"), stdout);
        }

        QCoreApplication::exit(EXIT_CODE_MISSING_REQUIRED_ARGUMENTS);
        return;
    }

    //Add SMP version command line
    entries.append({{&option_smp_v1, &option_smp_v2}, false, true});

    user_transport = parser.value(option_transport);

    if (0)
    {
    }
#if defined(PLUGIN_MCUMGR_TRANSPORT_UART)
    else if (user_transport == value_transport_uart)
    {
        add_transport_options_uart(&entries);
    }
#endif
#if defined(PLUGIN_MCUMGR_TRANSPORT_BLUETOOTH)
    else if (user_transport == value_transport_bluetooth)
    {
        add_transport_options_bluetooth(&entries);
    }
#endif
#if defined(PLUGIN_MCUMGR_TRANSPORT_UDP)
    else if (user_transport == value_transport_udp)
    {
        add_transport_options_udp(&entries);
    }
#endif
#if defined(PLUGIN_MCUMGR_TRANSPORT_LORAWAN)
    else if (user_transport == value_transport_lorawan)
    {
        add_transport_options_lorawan(&entries);
    }
#endif
    else
    {
        fputs(qPrintable(tr("Error: invalid transport specified")), stdout);
        QCoreApplication::exit(EXIT_CODE_INVALID_TRANSPORT);
        return;
    }

    user_group = parser.value(option_group);

    if (0)
    {
    }
    else if (user_group == value_group_enum)
    {
        add_group_options_enum(&entries, parser.value(option_command));
    }
    else if (user_group == value_group_fs)
    {
        add_group_options_fs(&entries, parser.value(option_command));
    }
    else if (user_group == value_group_os)
    {
        add_group_options_os(&entries, parser.value(option_command));
    }
    else if (user_group == value_group_settings)
    {
        add_group_options_settings(&entries, parser.value(option_command));
    }
    else if (user_group == value_group_shell)
    {
        add_group_options_shell(&entries, parser.value(option_command));
    }
    else if (user_group == value_group_stat)
    {
        add_group_options_stat(&entries, parser.value(option_command));
    }
    else if (user_group == value_group_zephyr)
    {
        add_group_options_zephyr(&entries, parser.value(option_command));
    }
    else if (user_group == value_group_img)
    {
        add_group_options_img(&entries, parser.value(option_command));
    }
    else
    {
        fputs(qPrintable(tr("Error: invalid group specified")), stdout);
        QCoreApplication::exit(EXIT_CODE_INVALID_GROUP);
        return;
    }

    //Add all entries to the parser
    i = 0;
    l = entries.length();

    while (i < l)
    {
        uint8_t c = 0;
        uint8_t m = entries[i].option.length();

        while (c < m)
        {
            parser.addOption(*entries[i].option[c]);
            ++c;
        }

        ++i;
    }

    if (parser.isSet(option_help))
    {
        fputs(qPrintable(parser.helpText()), stdout);
        QCoreApplication::exit(EXIT_CODE_SUCCESS);
        return;
    }

    //Re-parse command line arguments after transport/group/command options have been added
    parser.parse(QCoreApplication::arguments());

    if (parser.unknownOptionNames().length() > 0 || parser.positionalArguments().length() > 0)
    {
        fputs(qPrintable(tr("Unknown arguments provided: ") % (QStringList() << parser.unknownOptionNames() << parser.positionalArguments()).join(", ")), stdout);
        QCoreApplication::exit(EXIT_CODE_UNKNOWN_ARGUMENTS_PROVIDED);
        return;
    }

    //Check for any missing required arguments or double exclusives
    i = 0;
    l = entries.length();

    while (i < l)
    {
        if (entries[i].required == true || entries[i].exclusive == true)
        {
            bool option_present = false;
            bool exclusivity_breached = false;
            uint8_t c = 0;
            uint8_t m = entries[i].option.length();

            while (c < m)
            {
                if (parser.isSet(*entries[i].option[c]) == true)
                {
                    if (entries[i].exclusive == true && option_present == true)
                    {
                        //We already have an exclusive option so this configuration is not allowed
                        exclusivity_breached = true;
                        break;
                    }

                    option_present = true;
                }

                ++c;
            }

            if (entries[i].required == true && option_present == false)
            {
                //Required option not present, output error with list of options
                QString required_options;

                c = 0;
                m = entries[i].option.length();

                while (c < m)
                {
                    required_options.append(entries[i].option[c]->names().join(" or --"));
                    ++c;

                    if (c < m)
                    {
                        required_options.append(" or --");
                    }
                }

                fputs(qPrintable(tr("Missing required argument: ") % "--" % required_options % "\n"), stdout);
                failed = true;
            }
            else if (exclusivity_breached == true)
            {
                //Multiple exclusive options used, output error with details
                QString conflicting_options;

                c = 0;
                m = entries[i].option.length();

                while (c < m)
                {
                    if (parser.isSet(*entries[i].option[c]))
                    {
                        conflicting_options.append(entries[i].option[c]->names().join(" and --")).append(" and --");
                    }

                    ++c;
                }

                conflicting_options.remove((conflicting_options.length() - 7), 7);
                fputs(qPrintable(tr("Conflicting exclusive arguments: ") % "--" % conflicting_options % "\n"), stdout);
                failed = true;
            }
        }

        ++i;
    }

    if (failed == true)
    {
        QCoreApplication::exit(EXIT_CODE_MISSING_REQUIRED_ARGUMENTS);
        return;
    }

//TODO: Check that options supplied for each transport/group are valid

    //Apply SMP parameters
    if (parser.isSet(option_mtu) == true)
    {
        smp_mtu = parser.value(option_mtu).toUInt();

//TODO: consts
        if (smp_mtu < 96 || smp_mtu > 16384)
        {
            fputs(qPrintable(tr("Argument out of range: ") % "--" % option_mtu.names().first() % "\n"), stdout);
            QCoreApplication::exit(EXIT_CODE_NUMERIAL_ARGUMENT_OUT_OF_RANGE);
            return;
        }
    }

    if (parser.isSet(option_smp_v1) == true)
    {
        smp_v2 = false;
    }
    else if (parser.isSet(option_smp_v2) == true)
    {
        smp_v2 = true;
    }

    //Set up and open transport
    if (0)
    {
    }
#if defined(PLUGIN_MCUMGR_TRANSPORT_UART)
    else if (user_transport == value_transport_uart)
    {
        transport_uart = new smp_uart(this);
        active_transport = transport_uart;

        exit_code = configure_transport_options_uart(transport_uart, &parser);

        if (exit_code != EXIT_CODE_SUCCESS)
        {
            QCoreApplication::exit(exit_code);
            return;
        }
    }
#endif
#if defined(PLUGIN_MCUMGR_TRANSPORT_BLUETOOTH)
    else if (user_transport == value_transport_bluetooth)
    {
        transport_bluetooth = new smp_bluetooth(this);
        active_transport = transport_bluetooth;

        exit_code = configure_transport_options_bluetooth(transport_bluetooth, &parser);

        if (exit_code != EXIT_CODE_SUCCESS)
        {
            QCoreApplication::exit(exit_code);
            return;
        }
    }
#endif
#if defined(PLUGIN_MCUMGR_TRANSPORT_UDP)
    else if (user_transport == value_transport_udp)
    {
        transport_udp = new smp_udp(this);
        active_transport = transport_udp;

        exit_code = configure_transport_options_udp(transport_udp, &parser);

        if (exit_code != EXIT_CODE_SUCCESS)
        {
            QCoreApplication::exit(exit_code);
            return;
        }
    }
#endif
#if defined(PLUGIN_MCUMGR_TRANSPORT_LORAWAN)
    else if (user_transport == value_transport_lorawan)
    {
        transport_lorawan = new smp_lorawan(this);
        active_transport = transport_lorawan;

        exit_code = configure_transport_options_lorawan(transport_lorawan, &parser);

        if (exit_code != EXIT_CODE_SUCCESS)
        {
            QCoreApplication::exit(exit_code);
            return;
        }
    }
#endif

    //Open transport, exit if it failed
    connect(active_transport, SIGNAL(connected()), this, SLOT(transport_connected()));
    connect(active_transport, SIGNAL(disconnected()), this, SLOT(transport_disconnected()));
    connect(active_transport, SIGNAL(connected()), &wait_loop, SLOT(quit()));
    exit_code = active_transport->connect();

    if (exit_code != SMP_TRANSPORT_ERROR_OK)
    {
        fputs(qPrintable(tr("Transport open failed: ") % QString::number(exit_code) % "\n"), stdout);
        QCoreApplication::exit(EXIT_CODE_TRANSPORT_OPEN_FAILED);
        return;
    }

    if (active_transport->is_connected() == false)
    {
        //Wait for connection to establish
//TODO: needs to also wait on error and have a timeout
        wait_loop.exec();
    }

    disconnect(active_transport, SIGNAL(connected()), &wait_loop, SLOT(quit()));

    //Issue specified command
    processor = new smp_processor(this);
    connect(active_transport, SIGNAL(receive_waiting(smp_message*)), processor, SLOT(message_received(smp_message*)));
    //connect(processor, SIGNAL(custom_message_callback(custom_message_callback_t,smp_error_t*)), this, SLOT(custom_message_callback(custom_message_callback_t,smp_error_t*)));

    if (0)
    {
    }
    else if (user_group == value_group_enum)
    {
        group_enum = new smp_group_enum_mgmt(processor);
        active_group = group_enum;

        connect(group_enum, SIGNAL(status(uint8_t,group_status,QString)), this, SLOT(status(uint8_t,group_status,QString)));
        connect(group_enum, SIGNAL(progress(uint8_t,uint8_t)), this, SLOT(progress(uint8_t,uint8_t)));

        exit_code = run_group_enum(&parser, parser.value(option_command));
    }
    else if (user_group == value_group_fs)
    {
        group_fs = new smp_group_fs_mgmt(processor);
        active_group = group_fs;

        connect(group_fs, SIGNAL(status(uint8_t,group_status,QString)), this, SLOT(status(uint8_t,group_status,QString)));
        connect(group_fs, SIGNAL(progress(uint8_t,uint8_t)), this, SLOT(progress(uint8_t,uint8_t)));

        exit_code = run_group_fs(&parser, parser.value(option_command));
    }
    else if (user_group == value_group_os)
    {
        group_os = new smp_group_os_mgmt(processor);
        active_group = group_os;

        connect(group_os, SIGNAL(status(uint8_t,group_status,QString)), this, SLOT(status(uint8_t,group_status,QString)));
        connect(group_os, SIGNAL(progress(uint8_t,uint8_t)), this, SLOT(progress(uint8_t,uint8_t)));

        exit_code = run_group_os(&parser, parser.value(option_command));
    }
    else if (user_group == value_group_settings)
    {
        group_settings = new smp_group_settings_mgmt(processor);
        active_group = group_settings;

        connect(group_settings, SIGNAL(status(uint8_t,group_status,QString)), this, SLOT(status(uint8_t,group_status,QString)));
        connect(group_settings, SIGNAL(progress(uint8_t,uint8_t)), this, SLOT(progress(uint8_t,uint8_t)));

        exit_code = run_group_settings(&parser, parser.value(option_command));
    }
    else if (user_group == value_group_shell)
    {
        group_shell = new smp_group_shell_mgmt(processor);
        active_group = group_shell;

        connect(group_shell, SIGNAL(status(uint8_t,group_status,QString)), this, SLOT(status(uint8_t,group_status,QString)));
        connect(group_shell, SIGNAL(progress(uint8_t,uint8_t)), this, SLOT(progress(uint8_t,uint8_t)));

        exit_code = run_group_shell(&parser, parser.value(option_command));
    }
    else if (user_group == value_group_stat)
    {
        group_stat = new smp_group_stat_mgmt(processor);
        active_group = group_stat;

        connect(group_stat, SIGNAL(status(uint8_t,group_status,QString)), this, SLOT(status(uint8_t,group_status,QString)));
        connect(group_stat, SIGNAL(progress(uint8_t,uint8_t)), this, SLOT(progress(uint8_t,uint8_t)));

        exit_code = run_group_stat(&parser, parser.value(option_command));
    }
    else if (user_group == value_group_zephyr)
    {
        group_zephyr = new smp_group_zephyr_mgmt(processor);
        active_group = group_zephyr;

        connect(group_zephyr, SIGNAL(status(uint8_t,group_status,QString)), this, SLOT(status(uint8_t,group_status,QString)));
        connect(group_zephyr, SIGNAL(progress(uint8_t,uint8_t)), this, SLOT(progress(uint8_t,uint8_t)));

        exit_code = run_group_zephyr(&parser, parser.value(option_command));
    }
    else if (user_group == value_group_img)
    {
        group_img = new smp_group_img_mgmt(processor);
        active_group = group_img;

        connect(group_img, SIGNAL(status(uint8_t,group_status,QString)), this, SLOT(status(uint8_t,group_status,QString)));
        connect(group_img, SIGNAL(progress(uint8_t,uint8_t)), this, SLOT(progress(uint8_t,uint8_t)));

        exit_code = run_group_img(&parser, parser.value(option_command));
    }

    if (exit_code != EXIT_CODE_SUCCESS)
    {
        QCoreApplication::exit(exit_code);
    }
}

#if defined(PLUGIN_MCUMGR_TRANSPORT_UART)
void command_processor::add_transport_options_uart(QList<entry_t> *entries)
{
    entries->append({{&option_transport_uart_port}, true, false});
    entries->append({{&option_transport_uart_baud}, false, false});
    entries->append({{&option_transport_uart_flow_control}, false, false});
    entries->append({{&option_transport_uart_parity}, false, false});
    entries->append({{&option_transport_uart_data_bits}, false, false});
    entries->append({{&option_transport_uart_stop_bits}, false, false});
}

int command_processor::configure_transport_options_uart(smp_uart *transport, QCommandLineParser *parser)
{
    struct smp_uart_config_t uart_configuration;

    uart_configuration.port_name = parser->value(option_transport_uart_port);

    if (parser->isSet(option_transport_uart_baud) == true)
    {
        bool converted = false;

        uart_configuration.baud = parser->value(option_transport_uart_baud).toUInt(&converted);

        if (converted == false)
        {
            return EXIT_CODE_NUMERIAL_ARGUMENT_CONVERSION_FAILED;
        }
    }
    else
    {
        uart_configuration.baud = 115200;
    }

    if (parser->isSet(option_transport_uart_flow_control) == true)
    {
        if (parser->value(option_transport_uart_flow_control) == "none")
        {
            uart_configuration.flow_control = SMP_UART_FLOW_CONTROL_NONE;
        }
        else if (parser->value(option_transport_uart_flow_control) == "hardware")
        {
            uart_configuration.flow_control = SMP_UART_FLOW_CONTROL_HARDWARE;
        }
        else if (parser->value(option_transport_uart_flow_control) == "software")
        {
            uart_configuration.flow_control = SMP_UART_FLOW_CONTROL_SOFTWARE;
        }
        else
        {
            return EXIT_CODE_ARGUMENT_VALUE_NOT_VALID;
        }
    }
    else
    {
        uart_configuration.flow_control = SMP_UART_FLOW_CONTROL_NONE;
    }

    if (parser->isSet(option_transport_uart_parity) == true)
    {
        if (parser->value(option_transport_uart_parity) == "none")
        {
            uart_configuration.parity = SMP_UART_PARITY_NONE;
        }
        else if (parser->value(option_transport_uart_parity) == "even")
        {
            uart_configuration.parity = SMP_UART_PARITY_EVEN;
        }
        else if (parser->value(option_transport_uart_parity) == "odd")
        {
            uart_configuration.parity = SMP_UART_PARITY_ODD;
        }
        else if (parser->value(option_transport_uart_parity) == "space")
        {
            uart_configuration.parity = SMP_UART_PARITY_SPACE;
        }
        else if (parser->value(option_transport_uart_parity) == "mark")
        {
            uart_configuration.parity = SMP_UART_PARITY_MARK;
        }
        else
        {
            return EXIT_CODE_ARGUMENT_VALUE_NOT_VALID;
        }
    }
    else
    {
        uart_configuration.parity = SMP_UART_PARITY_NONE;
    }

    if (parser->isSet(option_transport_uart_data_bits) == true)
    {
        bool converted = false;
        uint8_t data_bits = parser->value(option_transport_uart_data_bits).toUInt(&converted);

        if (converted == false)
        {
            return EXIT_CODE_NUMERIAL_ARGUMENT_CONVERSION_FAILED;
        }
        else if (data_bits == 7)
        {
            uart_configuration.data_bits = SMP_UART_DATA_BITS_7;
        }
        else if (data_bits == 8)
        {
            uart_configuration.data_bits = SMP_UART_DATA_BITS_8;
        }
        else
        {
            return EXIT_CODE_NUMERIAL_ARGUMENT_OUT_OF_RANGE;
        }
    }
    else
    {
        uart_configuration.data_bits = SMP_UART_DATA_BITS_8;
    }

    if (parser->isSet(option_transport_uart_stop_bits) == true)
    {
        if (parser->value(option_transport_uart_stop_bits) == "1")
        {
            uart_configuration.stop_bits = SMP_UART_STOP_BITS_1;
        }
        else if (parser->value(option_transport_uart_stop_bits) == "1.5")
        {
            uart_configuration.stop_bits = SMP_UART_STOP_BITS_1_AND_HALF;
        }
        else if (parser->value(option_transport_uart_stop_bits) == "2")
        {
            uart_configuration.stop_bits = SMP_UART_STOP_BITS_2;
        }
        else
        {
            return EXIT_CODE_NUMERIAL_ARGUMENT_OUT_OF_RANGE;
        }
    }
    else
    {
        uart_configuration.stop_bits = SMP_UART_STOP_BITS_1;
    }

    transport->set_connection_config(&uart_configuration);

    return EXIT_CODE_SUCCESS;
}
#endif

#if defined(PLUGIN_MCUMGR_TRANSPORT_BLUETOOTH)
void command_processor::add_transport_options_bluetooth(QList<entry_t> *entries)
{
    //TODO: make exclusive
    entries->append({{&option_transport_bluetooth_name}, false, false});
    entries->append({{&option_transport_bluetooth_address}, false, false});
}

int command_processor::configure_transport_options_bluetooth(smp_bluetooth *transport, QCommandLineParser *parser)
{
    struct smp_bluetooth_config_t bluetooth_configuration;

    if (parser->isSet(option_transport_bluetooth_name) == true)
    {
        bluetooth_configuration.name = parser->value(option_transport_bluetooth_name);
        bluetooth_configuration.type = SMP_BLUETOOTH_CONNECT_TYPE_NAME;
    }
    else if (parser->isSet(option_transport_bluetooth_address) == true)
    {
        bluetooth_configuration.address = parser->value(option_transport_bluetooth_address);
        bluetooth_configuration.type = SMP_BLUETOOTH_CONNECT_TYPE_ADDRESS;
    }

    transport->set_connection_config(&bluetooth_configuration);

    return EXIT_CODE_SUCCESS;
}
#endif

#if defined(PLUGIN_MCUMGR_TRANSPORT_UDP)
void command_processor::add_transport_options_udp(QList<entry_t> *entries)
{
    entries->append({{&option_transport_udp_host}, true, false});
    entries->append({{&option_transport_udp_port}, false, false});
}

int command_processor::configure_transport_options_udp(smp_udp *transport, QCommandLineParser *parser)
{
    struct smp_udp_config_t udp_configuration;

    udp_configuration.hostname = parser->value(option_transport_udp_host);

    if (parser->isSet(option_transport_udp_port) == true)
    {
        bool converted = false;

        udp_configuration.port = parser->value(option_transport_udp_port).toUInt(&converted);

        if (converted == false)
        {
            return EXIT_CODE_NUMERIAL_ARGUMENT_CONVERSION_FAILED;
        }
    }
    else
    {
        udp_configuration.port = 1337;
    }

    transport->set_connection_config(&udp_configuration);

    return EXIT_CODE_SUCCESS;
}
#endif

#if defined(PLUGIN_MCUMGR_TRANSPORT_LORAWAN)
void command_processor::add_transport_options_lorawan(QList<entry_t> *entries)
{
    entries->append({{&option_transport_lorawan_host}, true, false});
    entries->append({{&option_transport_lorawan_port}, false, false});
    entries->append({{&option_transport_lorawan_tls, &option_transport_lorawan_no_tls}, false, true});
    entries->append({{&option_transport_lorawan_username}, true, false});
    entries->append({{&option_transport_lorawan_password}, true, false});
    entries->append({{&option_transport_lorawan_topic}, true, false});
    entries->append({{&option_transport_lorawan_frame_port}, true, false});
}

int command_processor::configure_transport_options_lorawan(smp_lorawan *transport, QCommandLineParser *parser)
{
    struct smp_lorawan_config_t lorawan_configuration;
    bool converted = false;

    lorawan_configuration.hostname = parser->value(option_transport_lorawan_host);
    lorawan_configuration.username = parser->value(option_transport_lorawan_username);
    lorawan_configuration.password = parser->value(option_transport_lorawan_password);
    lorawan_configuration.topic = parser->value(option_transport_lorawan_topic);

    if (parser->isSet(option_transport_lorawan_tls) == true)
    {
        lorawan_configuration.tls = true;
    }
    else if (parser->isSet(option_transport_lorawan_no_tls) == true)
    {
        lorawan_configuration.tls = false;
    }
    else
    {
        lorawan_configuration.tls = true;
    }

    if (parser->isSet(option_transport_lorawan_port) == true)
    {
        lorawan_configuration.port = parser->value(option_transport_lorawan_port).toUInt(&converted);

        if (converted == false)
        {
            return EXIT_CODE_NUMERIAL_ARGUMENT_CONVERSION_FAILED;
        }
    }
    else
    {
        if (lorawan_configuration.tls == true)
        {
            lorawan_configuration.port = 8883;
        }
        else
        {
            lorawan_configuration.port = 1883;
        }
    }

    converted = false;

    lorawan_configuration.frame_port = parser->value(option_transport_lorawan_frame_port).toUInt(&converted);

    if (converted == false)
    {
        return EXIT_CODE_NUMERIAL_ARGUMENT_CONVERSION_FAILED;
    }

    transport->set_connection_config(&lorawan_configuration);

    return EXIT_CODE_SUCCESS;
}
#endif

void command_processor::add_group_options_enum(QList<entry_t> *entries, QString command)
{
    if (command.length() == 0)
    {
        return;
    }

    if (command == value_command_enum_count)
    {
    }
    else if (command == value_command_enum_list)
    {
    }
    else if (command == value_command_enum_single)
    {
        //index
        entries->append({{&option_command_enum_index}, true, false});
    }
    else if (command == value_command_enum_details)
    {
        //groups (array)
        entries->append({{&option_command_enum_groups}, true, false});
    }
}

void command_processor::add_group_options_fs(QList<entry_t> *entries, QString command)
{
    if (command.length() == 0)
    {
        return;
    }

    if (command == value_command_fs_upload || command == value_command_fs_download)
    {
        //local file, remote file
        entries->append({{&option_command_fs_local_file}, true, false});
        entries->append({{&option_command_fs_remote_file}, true, false});
    }
    else if (command == value_command_fs_status)
    {
        //remote file
        entries->append({{&option_command_fs_remote_file}, true, false});
    }
    else if (value_command_fs_hash_checksum.contains(command))
    {
        //remote file, hash/checksum
        entries->append({{&option_command_fs_remote_file}, true, false});
        entries->append({{&option_command_fs_hash_checksum}, true, false});
    }
    else if (value_command_fs_supported_hashes_checksums.contains(command))
    {
    }
    else if (command == value_command_fs_close)
    {
    }
}

void command_processor::add_group_options_os(QList<entry_t> *entries, QString command)
{
    if (command.length() == 0)
    {
        return;
    }

    if (command == value_command_os_echo)
    {
        //data
        entries->append({{&option_command_os_data}, true, false});
    }
    else if (command == value_command_os_tasks)
    {
    }
    else if (command == value_command_os_memory)
    {
    }
    else if (command == value_command_os_reset)
    {
        //force
        entries->append({{&option_command_os_force}, false, false});
    }
    else if (command == value_command_os_mcumgr_parameters)
    {
    }
    else if (command == value_command_os_application_info)
    {
        //format
        entries->append({{&option_command_os_format}, false, false});
    }
    else if (command == value_command_os_get_date_time)
    {
    }
    else if (command == value_command_os_set_date_time)
    {
        //datetime
        entries->append({{&option_command_os_datetime}, true, false});
    }
    else if (command == value_command_os_bootloader_info)
    {
        //query
        entries->append({{&option_command_os_query}, false, false});
    }
}

void command_processor::add_group_options_settings(QList<entry_t> *entries, QString command)
{
    if (command.length() == 0)
    {
        return;
    }
}

void command_processor::add_group_options_shell(QList<entry_t> *entries, QString command)
{
    if (command.length() == 0)
    {
        return;
    }
}

void command_processor::add_group_options_stat(QList<entry_t> *entries, QString command)
{
    if (command.length() == 0)
    {
        return;
    }
}

void command_processor::add_group_options_zephyr(QList<entry_t> *entries, QString command)
{
    if (command.length() == 0)
    {
        return;
    }
}

void command_processor::add_group_options_img(QList<entry_t> *entries, QString command)
{
    if (command.length() == 0)
    {
        return;
    }

    if (command == value_command_img_get_state)
    {
    }
    else if (command == value_command_img_set_state)
    {
        //hash, confirm
//TODO: need to check if supplied parameters are valid
        entries->append({{&option_command_img_hash}, false, false});
        entries->append({{&option_command_img_confirm}, false, false});
    }
    else if (command == value_command_img_upload)
    {
        //image, file, upgrade
        entries->append({{&option_command_img_image}, false, false});
        entries->append({{&option_command_img_file}, true, false});
        entries->append({{&option_command_img_upgrade}, false, false});
    }
    else if (command == value_command_img_erase)
    {
        //slot
        entries->append({{&option_command_img_slot}, true, false});
    }
    else if (command == value_command_img_slot_info)
    {
    }
}

int command_processor::run_group_enum(QCommandLineParser *parser, QString command)
{
    //TODO
    return EXIT_CODE_SUCCESS;
}

int command_processor::run_group_fs(QCommandLineParser *parser, QString command)
{
    //TODO
    return EXIT_CODE_SUCCESS;
}

int command_processor::run_group_os(QCommandLineParser *parser, QString command)
{
    if (command == value_command_os_echo)
    {
        //data
        mode = ACTION_OS_ECHO;
        processor->set_transport(active_transport);
        set_group_transport_settings(active_group);
        group_os->start_echo(parser->value(option_command_os_data));
    }
    else if (command == value_command_os_tasks)
    {
        //TODO
    }
    else if (command == value_command_os_memory)
    {
        //TODO
    }
    else if (command == value_command_os_reset)
    {
        //force
        mode = ACTION_OS_RESET;
        processor->set_transport(active_transport);
        set_group_transport_settings(active_group);
        group_os->start_reset(parser->isSet(option_command_os_force));
    }
    else if (command == value_command_os_mcumgr_parameters)
    {
        //TODO
    }
    else if (command == value_command_os_application_info)
    {
        //TODO
        //format
        //option_command_os_format
    }
    else if (command == value_command_os_get_date_time)
    {
        //TODO
    }
    else if (command == value_command_os_set_date_time)
    {
        //TODO
        //datetime
        //option_command_os_datetime
    }
    else if (command == value_command_os_bootloader_info)
    {
        //TODO
        //query
        //option_command_os_query
    }

    return EXIT_CODE_SUCCESS;
}

int command_processor::run_group_settings(QCommandLineParser *parser, QString command)
{
    //TODO
    return EXIT_CODE_SUCCESS;
}

int command_processor::run_group_shell(QCommandLineParser *parser, QString command)
{
    //TODO
    return EXIT_CODE_SUCCESS;
}

int command_processor::run_group_stat(QCommandLineParser *parser, QString command)
{
    //TODO
    return EXIT_CODE_SUCCESS;
}

int command_processor::run_group_zephyr(QCommandLineParser *parser, QString command)
{
    //TODO
    return EXIT_CODE_SUCCESS;
}

int command_processor::run_group_img(QCommandLineParser *parser, QString command)
{
    if (command == value_command_img_get_state)
    {
        mode = ACTION_IMG_IMAGE_LIST;
        processor->set_transport(active_transport);
        set_group_transport_settings(active_group);
        img_mgmt_get_state_images = new QList<image_state_t>();
        group_img->start_image_get(img_mgmt_get_state_images);
    }
    else if (command == value_command_img_set_state)
    {
        QByteArray hash;

        if (parser->isSet(option_command_img_hash))
        {
            hash = QByteArray::fromHex(parser->value(option_command_img_hash).toLatin1());
        }

        mode = ACTION_IMG_IMAGE_SET;
        processor->set_transport(active_transport);
        set_group_transport_settings(active_group);
        group_img->start_image_set((parser->isSet(option_command_img_hash) ? &hash : nullptr), (parser->isSet(option_command_img_confirm) ? true : false), nullptr);
    }
    else if (command == value_command_img_upload)
    {
        QByteArray hash;

        mode = ACTION_IMG_UPLOAD;
        processor->set_transport(active_transport);
        set_group_transport_settings(active_group);
        group_img->start_firmware_update((parser->isSet(option_command_img_image) ? parser->value(option_command_img_image).toUInt() : 0), parser->value(option_command_img_file), (parser->isSet(option_command_img_upgrade) ? true : false), &hash);
    }
    else if (command == value_command_img_erase)
    {
        mode = ACTION_IMG_IMAGE_ERASE;
        processor->set_transport(active_transport);
        set_group_transport_settings(active_group);
        group_img->start_image_erase(parser->value(option_command_img_slot).toUInt());
    }
    else if (command == value_command_img_slot_info)
    {
        //TODO
        mode = ACTION_IMG_IMAGE_SLOT_INFO;
        processor->set_transport(active_transport);
        set_group_transport_settings(active_group);
        img_mgmt_slot_info_images = new QList<slot_info_t>();
        group_img->start_image_slot_info(img_mgmt_slot_info_images);
    }

    return EXIT_CODE_SUCCESS;
}

void command_processor::set_group_transport_settings(smp_group *group)
{
    group->set_parameters(smp_v2, smp_mtu, active_transport->get_retries(), active_transport->get_timeout(), mode);
}

void command_processor::set_group_transport_settings(smp_group *group, uint32_t timeout)
{
    group->set_parameters(smp_v2, smp_mtu, active_transport->get_retries(), (timeout >= active_transport->get_timeout() ? timeout : active_transport->get_timeout()), mode);
}

void command_processor::status(uint8_t user_data, group_status status, QString error_string)
{
#if 0
    if (error_string != nullptr)
    {
        qDebug() << "status: " << user_data << ", " << status << ", " << error_string;
    }
    else
    {
        qDebug() << "status: " << user_data << ", " << status;
    }
#endif

//    QLabel *label_status = nullptr;
    bool finished = true;
    bool skip_error_string = false;

//    log_debug() << "Status: " << status;

    if (sender() == group_img)
    {
        log_debug() << "img sender";

        if (status == STATUS_COMPLETE)
        {
            log_debug() << "complete";

            //Advance to next stage of image upload
            if (user_data == ACTION_IMG_UPLOAD)
            {
                log_debug() << "is upload";
#if 0
                if (radio_IMG_Test->isChecked() || radio_IMG_Confirm->isChecked())
                {
                    //Mark image for test or confirmation
                    finished = false;

                    mode = ACTION_IMG_UPLOAD_SET;
                    processor->set_transport(active_transport());
                    set_group_transport_settings(smp_groups.img_mgmt);
                    bool started = smp_groups.img_mgmt->start_image_set(&upload_hash, (radio_IMG_Confirm->isChecked() ? true : false), nullptr);
                    //todo: check status

                    log_debug() << "do upload of " << upload_hash;
                }
#endif
            }
            else if (user_data == ACTION_IMG_UPLOAD_SET)
            {
#if 0
                if (check_IMG_Reset->isChecked())
                {
                    //Reboot device
                    finished = false;

                    mode = ACTION_OS_UPLOAD_RESET;
                    processor->set_transport(active_transport());
                    set_group_transport_settings(smp_groups.os_mgmt);
                    bool started = smp_groups.os_mgmt->start_reset(false);
                    //todo: check status

                    log_debug() << "do reset";
                }
#endif
            }
            else if (user_data == ACTION_IMG_IMAGE_LIST)
            {
                uint8_t i = 0;
                uint8_t l = (*img_mgmt_get_state_images).length();

                while (i < l)
                {
                    uint8_t c = 0;
                    uint8_t m = (*img_mgmt_get_state_images)[i].slot_list.length();

                    if ((*img_mgmt_get_state_images)[i].image_set == true)
                    {
                        fputs(qPrintable(tr("Image ") % QString::number((*img_mgmt_get_state_images)[i].image) % "\n"), stdout);
                    }
                    else
                    {
                        fputs(qPrintable(tr("Image (assumed ") % QString::number(i) % ")\n"), stdout);
                    }

                    while (c < m)
                    {
                        AutEscape::to_hex(&(*img_mgmt_get_state_images)[i].slot_list[c].hash);
                        fputs(qPrintable(tr("\tSlot ") % QString::number((*img_mgmt_get_state_images)[i].slot_list[c].slot) % "\n"), stdout);
                        fputs(qPrintable(tr("\t\tHash: ") % (*img_mgmt_get_state_images)[i].slot_list[c].hash % "\n"), stdout);
                        fputs(qPrintable(tr("\t\tVersion: ") % (*img_mgmt_get_state_images)[i].slot_list[c].version % "\n"), stdout);

                        if ((*img_mgmt_get_state_images)[i].slot_list[c].active == true)
                        {
                            fputs(qPrintable(tr("\t\t- Active") % "\n"), stdout);
                        }

                        if ((*img_mgmt_get_state_images)[i].slot_list[c].bootable == true)
                        {
                            fputs(qPrintable(tr("\t\t- Bootable") % "\n"), stdout);
                        }

                        if ((*img_mgmt_get_state_images)[i].slot_list[c].confirmed == true)
                        {
                            fputs(qPrintable(tr("\t\t- Confirmed") % "\n"), stdout);
                        }

                        if ((*img_mgmt_get_state_images)[i].slot_list[c].pending == true)
                        {
                            fputs(qPrintable(tr("\t\t- Pending") % "\n"), stdout);
                        }

                        if ((*img_mgmt_get_state_images)[i].slot_list[c].permanent == true)
                        {
                            fputs(qPrintable(tr("\t\t- Permanent") % "\n"), stdout);
                        }

                        if ((*img_mgmt_get_state_images)[i].slot_list[c].splitstatus == true)
                        {
                            fputs(qPrintable(tr("\t\t- Split image") % "\n"), stdout);
                        }

                        ++c;
                    }

                    ++i;
                }
            }
            else if (user_data == ACTION_IMG_IMAGE_SET)
            {
#if 0
                if (parent_row != -1 && parent_column != -1 && child_row != -1 && child_column != -1)
                {
                    uint8_t i = 0;

                    model_image_state.clear();

                    while (i < images_list.length())
                    {
                        model_image_state.appendRow(images_list[i].item);
                        ++i;
                    }

                    if (model_image_state.hasIndex(parent_row, parent_column) == true && model_image_state.index(child_row, child_column, model_image_state.index(parent_row, parent_column)).isValid() == true)
                    {
                        colview_IMG_Images->setCurrentIndex(model_image_state.index(child_row, child_column, model_image_state.index(parent_row, parent_column)));
                    }
                    else
                    {
                        colview_IMG_Images->previewWidget()->hide();
                    }

                    parent_row = -1;
                    parent_column = -1;
                    child_row = -1;
                    child_column = -1;
                }
                else
                {
                    colview_IMG_Images->previewWidget()->hide();
                }
#endif
            }
            else if (user_data == ACTION_IMG_IMAGE_SLOT_INFO)
            {
                uint8_t i = 0;
                uint8_t l = (*img_mgmt_slot_info_images).length();

                while (i < l)
                {
                    uint8_t c = 0;
                    uint8_t m = (*img_mgmt_slot_info_images)[i].slot_data.length();
                    QString field_size;

                    fputs(qPrintable(tr("Image ") % QString::number((*img_mgmt_slot_info_images)[i].image) % "\n"), stdout);

                    while (c < m)
                    {
                        fputs(qPrintable(tr("\tSlot ") % QString::number((*img_mgmt_slot_info_images)[i].slot_data[c].slot) % "\n"), stdout);

                        if ((*img_mgmt_slot_info_images)[i].slot_data[c].size_present == true)
                        {
                            size_abbreviation((*img_mgmt_slot_info_images)[i].slot_data[c].size, &field_size);
                            fputs(qPrintable(tr("\t\tSize: ") % field_size % "\n"), stdout);
                            field_size.clear();
                        }

                        if ((*img_mgmt_slot_info_images)[i].slot_data[c].upload_image_id_present == true)
                        {
                            fputs(qPrintable(tr("\t\tUpload image ID: ") % QString::number((*img_mgmt_slot_info_images)[i].slot_data[c].upload_image_id) % "\n"), stdout);
                        }

                        ++c;
                    }

                    if ((*img_mgmt_slot_info_images)[i].max_image_size_present == true)
                    {
                        size_abbreviation((*img_mgmt_slot_info_images)[i].max_image_size, &field_size);
                        fputs(qPrintable(tr("\tMax image size: ") % field_size % "\n"), stdout);
                        field_size.clear();
                    }

                    ++i;
                }
            }
        }
        else if (status == STATUS_UNSUPPORTED)
        {
            log_debug() << "unsupported";

            //Advance to next stage of image upload, this is likely to occur in MCUboot serial recovery whereby the image state functionality is not included
            if (user_data == ACTION_IMG_UPLOAD_SET)
            {
#if 0
                skip_error_string = true;

                if (check_IMG_Reset->isChecked())
                {
                    //Reboot device
                    finished = false;

                    mode = ACTION_OS_UPLOAD_RESET;
                    processor->set_transport(active_transport());
                    set_group_transport_settings(smp_groups.os_mgmt);
                    bool started = smp_groups.os_mgmt->start_reset(false);
                    //todo: check status

                    log_debug() << "do reset";

                    lbl_IMG_Status->setText("Resetting...");
                }
                else
                {
                    lbl_IMG_Status->setText("Upload finished, set image state failed: command not supported (likely MCUboot serial recovery)");
                }
#endif
            }
        }

        if (user_data == ACTION_IMG_IMAGE_SLOT_INFO)
        {
            delete img_mgmt_slot_info_images;
            img_mgmt_slot_info_images = nullptr;
        }
        else if (user_data == ACTION_IMG_IMAGE_LIST)
        {
            delete img_mgmt_get_state_images;
            img_mgmt_get_state_images = nullptr;
        }
    }
    else if (sender() == group_os)
    {
        log_debug() << "os sender";
//        label_status = lbl_OS_Status;

        if (status == STATUS_COMPLETE)
        {
            log_debug() << "complete";

            if (user_data == ACTION_OS_ECHO)
            {
//                edit_OS_Echo_Output->appendPlainText(error_string);
//                error_string = nullptr;
            }
            else if (user_data == ACTION_OS_UPLOAD_RESET)
            {
            }
            else if (user_data == ACTION_OS_RESET)
            {
            }
#if 0
            else if (user_data == ACTION_OS_MEMORY_POOL)
            {
                uint16_t i = 0;
                uint16_t l = table_OS_Memory->rowCount();

                table_OS_Memory->setSortingEnabled(false);

                while (i < memory_list.length())
                {
                    if (i >= l)
                    {
                        table_OS_Memory->insertRow(i);

                        QTableWidgetItem *row_name = new QTableWidgetItem(memory_list[i].name);
                        QTableWidgetItem *row_size = new QTableWidgetItem(QString::number(memory_list[i].blocks * memory_list[i].size));
                        QTableWidgetItem *row_free = new QTableWidgetItem(QString::number(memory_list[i].free * memory_list[i].size));
                        QTableWidgetItem *row_minimum = new QTableWidgetItem(QString::number(memory_list[i].minimum * memory_list[i].size));

                        table_OS_Memory->setItem(i, 0, row_name);
                        table_OS_Memory->setItem(i, 1, row_size);
                        table_OS_Memory->setItem(i, 2, row_free);
                        table_OS_Memory->setItem(i, 3, row_minimum);
                    }
                    else
                    {
                        table_OS_Memory->item(i, 0)->setText(memory_list[i].name);
                        table_OS_Memory->item(i, 1)->setText(QString::number(memory_list[i].blocks * memory_list[i].size));
                        table_OS_Memory->item(i, 2)->setText(QString::number(memory_list[i].free * memory_list[i].size));
                        table_OS_Memory->item(i, 3)->setText(QString::number(memory_list[i].minimum * memory_list[i].size));
                    }

                    ++i;
                }

                while (i < l)
                {
                    table_OS_Memory->removeRow((table_OS_Memory->rowCount() - 1));
                    ++i;
                }

                table_OS_Memory->setSortingEnabled(true);
            }
            else if (user_data == ACTION_OS_TASK_STATS)
            {
                uint16_t i = 0;
                uint16_t l = table_OS_Tasks->rowCount();

                table_OS_Tasks->setSortingEnabled(false);

                while (i < task_list.length())
                {
                    if (i >= l)
                    {
                        table_OS_Tasks->insertRow(i);

                        QTableWidgetItem *row_name = new QTableWidgetItem(task_list[i].name);
                        QTableWidgetItem *row_id = new QTableWidgetItem(QString::number(task_list[i].id));
                        QTableWidgetItem *row_priority = new QTableWidgetItem(QString::number(task_list[i].priority));
                        QTableWidgetItem *row_state = new QTableWidgetItem(QString::number(task_list[i].state));
                        QTableWidgetItem *row_context_switches = new QTableWidgetItem(QString::number(task_list[i].context_switches));
                        QTableWidgetItem *row_runtime = new QTableWidgetItem(QString::number(task_list[i].runtime));
                        QTableWidgetItem *row_stack_size = new QTableWidgetItem(QString::number(task_list[i].stack_size * 4));
                        QTableWidgetItem *row_stack_usage = new QTableWidgetItem(QString::number(task_list[i].stack_usage * 4));

                        table_OS_Tasks->setItem(i, 0, row_name);
                        table_OS_Tasks->setItem(i, 1, row_id);
                        table_OS_Tasks->setItem(i, 2, row_priority);
                        table_OS_Tasks->setItem(i, 3, row_state);
                        table_OS_Tasks->setItem(i, 4, row_context_switches);
                        table_OS_Tasks->setItem(i, 5, row_runtime);
                        table_OS_Tasks->setItem(i, 6, row_stack_size);
                        table_OS_Tasks->setItem(i, 7, row_stack_usage);
                    }
                    else
                    {
                        table_OS_Tasks->item(i, 0)->setText(task_list[i].name);
                        table_OS_Tasks->item(i, 1)->setText(QString::number(task_list[i].id));
                        table_OS_Tasks->item(i, 2)->setText(QString::number(task_list[i].priority));
                        table_OS_Tasks->item(i, 3)->setText(QString::number(task_list[i].state));
                        table_OS_Tasks->item(i, 4)->setText(QString::number(task_list[i].context_switches));
                        table_OS_Tasks->item(i, 5)->setText(QString::number(task_list[i].runtime));
                        table_OS_Tasks->item(i, 6)->setText(QString::number(task_list[i].stack_size * sizeof(uint32_t)));
                        table_OS_Tasks->item(i, 7)->setText(QString::number(task_list[i].stack_usage * sizeof(uint32_t)));
                    }

                    ++i;
                }

                while (i < l)
                {
                    table_OS_Tasks->removeRow((table_OS_Tasks->rowCount() - 1));
                    ++i;
                }

                table_OS_Tasks->setSortingEnabled(true);
            }
            else if (user_data == ACTION_OS_MCUMGR_BUFFER)
            {
                edit_OS_Info_Output->clear();
                edit_OS_Info_Output->appendPlainText(error_string);
                error_string = nullptr;
            }
            else if (user_data == ACTION_OS_OS_APPLICATION_INFO)
            {
                edit_OS_Info_Output->clear();
                edit_OS_Info_Output->appendPlainText(error_string);
                error_string = nullptr;
            }
            else if (user_data == ACTION_OS_BOOTLOADER_INFO)
            {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
                switch (bootloader_info_response.typeId())
#else
                switch (bootloader_info_response.type())
#endif
                {
                case QMetaType::Bool:
                {
                    edit_os_bootloader_response->setText(bootloader_info_response.toBool() == true ? "True" : "False");
                    break;
                }

                case QMetaType::Int:
                {
                    edit_os_bootloader_response->setText(QString::number(bootloader_info_response.toInt()));
                    break;
                }

                case QMetaType::LongLong:
                {
                    edit_os_bootloader_response->setText(QString::number(bootloader_info_response.toLongLong()));
                    break;
                }

                case QMetaType::UInt:
                {
                    edit_os_bootloader_response->setText(QString::number(bootloader_info_response.toUInt()));
                    break;
                }

                case QMetaType::ULongLong:
                {
                    edit_os_bootloader_response->setText(QString::number(bootloader_info_response.toULongLong()));
                    break;
                }

                case QMetaType::Double:
                {
                    edit_os_bootloader_response->setText(QString::number(bootloader_info_response.toDouble()));
                    break;
                }

                case QMetaType::QString:
                {
                    edit_os_bootloader_response->setText(bootloader_info_response.toString());
                    break;
                }

                default:
                {
                    error_string = "Invalid";
                }
                }
            }
            else if (user_data == ACTION_OS_DATETIME_GET)
            {
                int index;
                log_debug() << "RTC response: " << rtc_time_date_response;

                rtc_time_date_response.setTimeZone(rtc_time_date_response.timeZone());
                index = combo_os_datetime_timezone->findText(rtc_time_date_response.timeZone().displayName(rtc_time_date_response, QTimeZone::OffsetName));

                if (index >= 0)
                {
                    combo_os_datetime_timezone->setCurrentIndex(index);
                }

                edit_os_datetime_date_time->setDateTime(rtc_time_date_response);
            }
            else if (user_data == ACTION_OS_DATETIME_SET)
            {
            }
#endif
        }
    }
#if 0
    else if (sender() == group_shell)
    {
        log_debug() << "shell sender";
        label_status = lbl_SHELL_Status;

        if (status == STATUS_COMPLETE)
        {
            log_debug() << "complete";

            if (user_data == ACTION_SHELL_EXECUTE)
            {
                edit_SHELL_Output->add_dat_in_text(error_string.toUtf8());

                if (shell_rc == 0)
                {
                    error_string = nullptr;
                }
                else
                {
                    error_string = QString("Finished, error (ret): ").append(QString::number(shell_rc));
                }
            }
        }
    }
    else if (sender() == group_stat)
    {
        log_debug() << "stat sender";
        label_status = lbl_STAT_Status;

        if (status == STATUS_COMPLETE)
        {
            log_debug() << "complete";

            if (user_data == ACTION_STAT_GROUP_DATA)
            {
                uint16_t i = 0;
                uint16_t l = table_STAT_Values->rowCount();

                table_STAT_Values->setSortingEnabled(false);

                while (i < stat_list.length())
                {
                    if (i >= l)
                    {
                        table_STAT_Values->insertRow(i);

                        QTableWidgetItem *row_name = new QTableWidgetItem(stat_list[i].name);
                        QTableWidgetItem *row_value = new QTableWidgetItem(QString::number(stat_list[i].value));


                        table_STAT_Values->setItem(i, 0, row_name);
                        table_STAT_Values->setItem(i, 1, row_value);
                    }
                    else
                    {
                        table_STAT_Values->item(i, 0)->setText(stat_list[i].name);
                        table_STAT_Values->item(i, 1)->setText(QString::number(stat_list[i].value));
                    }

                    ++i;
                }

                while (i < l)
                {
                    table_STAT_Values->removeRow((table_STAT_Values->rowCount() - 1));
                    ++i;
                }

                table_STAT_Values->setSortingEnabled(true);
            }
            else if (user_data == ACTION_STAT_LIST_GROUPS)
            {
                combo_STAT_Group->clear();
                combo_STAT_Group->addItems(group_list);
            }
        }
    }
    else if (sender() == group_fs)
    {
        log_debug() << "fs sender";
        label_status = lbl_FS_Status;

        if (status == STATUS_COMPLETE)
        {
            log_debug() << "complete";

            if (user_data == ACTION_FS_UPLOAD)
            {
                //edit_FS_Log->appendPlainText("todo");
            }
            else if (user_data == ACTION_FS_DOWNLOAD)
            {
                //edit_FS_Log->appendPlainText("todo2");
            }
            else if (user_data == ACTION_FS_HASH_CHECKSUM)
            {
                error_string.prepend("Finished hash/checksum using ");
                edit_FS_Result->setText(fs_hash_checksum_response.toHex());
                edit_FS_Size->setText(QString::number(fs_size_response));
            }
            else if (user_data == ACTION_FS_SUPPORTED_HASHES_CHECKSUMS)
            {
                uint8_t i = 0;

                combo_FS_type->clear();

                while (i < supported_hash_checksum_list.length())
                {
                    combo_FS_type->addItem(supported_hash_checksum_list.at(i).name);
                    log_debug() << supported_hash_checksum_list.at(i).format << ", " << supported_hash_checksum_list.at(i).size;
                    ++i;
                }
            }
            else if (user_data == ACTION_FS_STATUS)
            {
                edit_FS_Size->setText(QString::number(fs_size_response));
            }
        }
    }
    else if (sender() == group_settings)
    {
        log_debug() << "settings sender";
        label_status = lbl_settings_status;

        if (status == STATUS_COMPLETE)
        {
            log_debug() << "complete";

            if (user_data == ACTION_SETTINGS_READ)
            {
                edit_settings_value->setText(settings_read_response.toHex());

                if (update_settings_display() == false)
                {
                    error_string = QString("Error: data is %1 bytes, cannot convert to decimal number").arg(QString::number(settings_read_response.length()));
                }
            }
            else if (user_data == ACTION_SETTINGS_WRITE || user_data == ACTION_SETTINGS_DELETE || user_data == ACTION_SETTINGS_COMMIT || user_data == ACTION_SETTINGS_LOAD || user_data == ACTION_SETTINGS_SAVE)
            {
            }
        }
    }
    else if (sender() == group_zephyr)
    {
        log_debug() << "zephyr sender";
        label_status = lbl_zephyr_status;

        if (user_data == ACTION_ZEPHYR_STORAGE_ERASE)
        {
        }
    }
    else if (sender() == group_enum)
    {
        log_debug() << "enum sender";
        label_status = lbl_enum_status;

        if (user_data == ACTION_ENUM_COUNT)
        {
            edit_Enum_Count->setText(QString::number(enum_count));
        }
        else if (user_data == ACTION_ENUM_LIST)
        {
            uint16_t i = 0;
            uint16_t l = table_Enum_List_Details->rowCount();

            table_Enum_List_Details->setSortingEnabled(false);

            while (i < enum_groups.length())
            {
                if (i >= l)
                {
                    table_Enum_List_Details->insertRow(i);

                    QTableWidgetItem *row_id = new QTableWidgetItem(QString::number(enum_groups[i]));
                    QTableWidgetItem *row_name = new QTableWidgetItem("");
                    QTableWidgetItem *row_handlers = new QTableWidgetItem("");

                    table_Enum_List_Details->setItem(i, 0, row_id);
                    table_Enum_List_Details->setItem(i, 1, row_name);
                    table_Enum_List_Details->setItem(i, 2, row_handlers);
                }
                else
                {
                    table_Enum_List_Details->item(i, 0)->setText(QString::number(enum_groups[i]));
                    table_Enum_List_Details->item(i, 1)->setText("");
                    table_Enum_List_Details->item(i, 2)->setText("");
                }

                ++i;
            }

            while (i < l)
            {
                table_Enum_List_Details->removeRow((table_Enum_List_Details->rowCount() - 1));
                ++i;
            }

            table_Enum_List_Details->setSortingEnabled(true);
        }
        else if (user_data == ACTION_ENUM_SINGLE)
        {
            edit_Enum_Count->setText(QString("ID: ").append(QString::number(enum_single_id)).append(", end: ").append(QString::number(enum_single_end)));
        }
        else if (user_data == ACTION_ENUM_DETAILS)
        {
            uint16_t i = 0;
            uint16_t l = table_Enum_List_Details->rowCount();

            table_Enum_List_Details->setSortingEnabled(false);

            while (i < enum_details.length())
            {
                if (i >= l)
                {
                    table_Enum_List_Details->insertRow(i);

                    QTableWidgetItem *row_id = new QTableWidgetItem(QString::number(enum_details[i].id));
                    QTableWidgetItem *row_name = new QTableWidgetItem(enum_details[i].name);
                    QTableWidgetItem *row_handlers = new QTableWidgetItem(QString::number(enum_details[i].handlers));

                    table_Enum_List_Details->setItem(i, 0, row_id);
                    table_Enum_List_Details->setItem(i, 1, row_name);
                    table_Enum_List_Details->setItem(i, 2, row_handlers);
                }
                else
                {
                    table_Enum_List_Details->item(i, 0)->setText(QString::number(enum_details[i].id));
                    table_Enum_List_Details->item(i, 1)->setText(enum_details[i].name);
                    table_Enum_List_Details->item(i, 2)->setText(QString::number(enum_details[i].handlers));
                }

                ++i;
            }

            while (i < l)
            {
                table_Enum_List_Details->removeRow((table_Enum_List_Details->rowCount() - 1));
                ++i;
            }

            table_Enum_List_Details->setSortingEnabled(true);
        }
    }
#endif

    if (finished == true)
    {
        mode = ACTION_IDLE;
        //relase_transport();

        if (error_string == nullptr)
        {
            if (status == STATUS_COMPLETE)
            {
                error_string = QString("Finished");
            }
            else if (status == STATUS_ERROR)
            {
                error_string = QString("Error");
            }
            else if (status == STATUS_TIMEOUT)
            {
                error_string = QString("Command timed out");
            }
            else if (status == STATUS_CANCELLED)
            {
                error_string = QString("Cancelled");
            }
        }
    }

    if (error_string != nullptr && skip_error_string == false)
    {
        qDebug() << error_string;
    }

    if (finished == true)
    {
        QCoreApplication::exit(EXIT_CODE_SUCCESS);
    }
}

void command_processor::progress(uint8_t user_data, uint8_t percent)
{
    qDebug() << "progress: " << user_data << ", " << percent;
}

void command_processor::transport_connected()
{
}

void command_processor::transport_disconnected()
{
}

void command_processor::size_abbreviation(uint32_t size, QString *output)
{
    const QStringList list_abbreviations = { "B", "KiB", "MiB", "GiB", "TiB" };
    float converted_size = size;
    uint8_t abbreviation_index = 0;

    while (converted_size >= 1024 && abbreviation_index < list_abbreviations.size())
    {
        converted_size /= 1024.0;
        ++abbreviation_index;
    }

    output->append(QString::number(converted_size, 'g', 3).append(list_abbreviations.at(abbreviation_index)));
}

/******************************************************************************/
// END OF FILE
/******************************************************************************/
