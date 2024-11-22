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
//const QCommandLineOption option_transport_lorawan_("port", "UART port", "port");

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

/******************************************************************************/
// Local Functions or Private Members
/******************************************************************************/
command_processor::command_processor(QObject *parent) : QObject{parent}
{
    processor = nullptr;
    group_os = nullptr;
    transport_uart = nullptr;
    active_transport = nullptr;
    mode = ACTION_IDLE;
    
    //TODO: Dummy values
    smp_v2 = true;
    smp_mtu = 256;

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

    if (active_transport != nullptr)
    {
        active_transport = nullptr;
    }

    if (processor != nullptr)
    {
        delete processor;
        processor = nullptr;
    }

    if (group_os != nullptr)
    {
        delete group_os;
        group_os = nullptr;
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

    parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
    parser.addOption(option_help);
    parser.addOption(option_transport);
    parser.addOption(option_group);
    parser.addOption(option_command);
    parser.parse(QCoreApplication::arguments());

    if (parser.isSet(option_version))
    {
        parser.showVersion();
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
        parser.addOption(*entries[i].option);
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
        if (entries[i].required == true && parser.isSet(*entries[i].option) == false)
        {
            fputs(qPrintable(tr("Missing required argument: ") % "--" % entries[i].option->names().join(" or --") % "\n"), stdout);
            failed = true;
        }

//TODO: exclusive
        ++i;
    }

    if (failed == true)
    {
        QCoreApplication::exit(EXIT_CODE_MISSING_REQUIRED_ARGUMENTS);
        return;
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
    }
#endif
#if defined(PLUGIN_MCUMGR_TRANSPORT_UDP)
    else if (user_transport == value_transport_udp)
    {
    }
#endif
#if defined(PLUGIN_MCUMGR_TRANSPORT_LORAWAN)
    else if (user_transport == value_transport_lorawan)
    {
    }
#endif

    //Open transport, exit if it failed
    exit_code = active_transport->connect();

    if (exit_code != SMP_TRANSPORT_ERROR_OK)
    {
        fputs(qPrintable(tr("Transport open failed: ") % QString::number(exit_code) % "\n"), stdout);
        QCoreApplication::exit(EXIT_CODE_TRANSPORT_OPEN_FAILED);
        return;
    }

    //Issue specified command
    processor = new smp_processor(this);
    connect(active_transport, SIGNAL(receive_waiting(smp_message*)), processor, SLOT(message_received(smp_message*)));
    //connect(processor, SIGNAL(custom_message_callback(custom_message_callback_t,smp_error_t*)), this, SLOT(custom_message_callback(custom_message_callback_t,smp_error_t*)));

    if (0)
    {
    }
    else if (user_group == value_group_enum)
    {

    }
    else if (user_group == value_group_fs)
    {

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

    }
    else if (user_group == value_group_shell)
    {

    }
    else if (user_group == value_group_stat)
    {

    }
    else if (user_group == value_group_zephyr)
    {

    }
    else if (user_group == value_group_img)
    {

    }

    if (exit_code != EXIT_CODE_SUCCESS)
    {
        QCoreApplication::exit(exit_code);
    }
}

#if defined(PLUGIN_MCUMGR_TRANSPORT_UART)
void command_processor::add_transport_options_uart(QList<entry_t> *entries)
{
    entries->append({&option_transport_uart_port, true, false, nullptr});
    entries->append({&option_transport_uart_baud, false, false, nullptr});
    entries->append({&option_transport_uart_flow_control, false, false, nullptr});
    entries->append({&option_transport_uart_parity, false, false, nullptr});
    entries->append({&option_transport_uart_data_bits, false, false, nullptr});
    entries->append({&option_transport_uart_stop_bits, false, false, nullptr});
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
    parser->addOption(option_transport_bluetooth_name);
    parser->addOption(option_transport_bluetooth_address);
}
#endif

#if defined(PLUGIN_MCUMGR_TRANSPORT_UDP)
void command_processor::add_transport_options_udp(QList<entry_t> *entries)
{
    parser->addOption(option_transport_udp_host);
    parser->addOption(option_transport_udp_port);
}
#endif

#if defined(PLUGIN_MCUMGR_TRANSPORT_LORAWAN)
void command_processor::add_transport_options_lorawan(QList<entry_t> *entries)
{
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
        entries->append({&option_command_enum_index, true, false, nullptr});
    }
    else if (command == value_command_enum_details)
    {
        //groups (array)
        entries->append({&option_command_enum_groups, true, false, nullptr});
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
        entries->append({&option_command_fs_local_file, true, false, nullptr});
        entries->append({&option_command_fs_remote_file, true, false, nullptr});
    }
    else if (command == value_command_fs_status)
    {
        //remote file
        entries->append({&option_command_fs_remote_file, true, false, nullptr});
    }
    else if (value_command_fs_hash_checksum.contains(command))
    {
        //remote file, hash/checksum
        entries->append({&option_command_fs_remote_file, true, false, nullptr});
        entries->append({&option_command_fs_hash_checksum, true, false, nullptr});
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
        entries->append({&option_command_os_data, true, false, nullptr});
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
        entries->append({&option_command_os_force, false, false, nullptr});
    }
    else if (command == value_command_os_mcumgr_parameters)
    {
    }
    else if (command == value_command_os_application_info)
    {
        //format
        entries->append({&option_command_os_format, false, false, nullptr});
    }
    else if (command == value_command_os_get_date_time)
    {
    }
    else if (command == value_command_os_set_date_time)
    {
        //datetime
        entries->append({&option_command_os_datetime, true, false, nullptr});
    }
    else if (command == value_command_os_bootloader_info)
    {
        //query
        entries->append({&option_command_os_query, false, false, nullptr});
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
}

#if 0
int command_processor::run_group_enum(QCommandLineParser *parser, QString command)
{
}

int command_processor::run_group_fs(QCommandLineParser *parser, QString command)
{
}
#endif

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
    }
    else if (command == value_command_os_memory)
    {
    }
    else if (command == value_command_os_reset)
    {
        //force
        //option_command_os_force
    }
    else if (command == value_command_os_mcumgr_parameters)
    {
    }
    else if (command == value_command_os_application_info)
    {
        //format
        //option_command_os_format
    }
    else if (command == value_command_os_get_date_time)
    {
    }
    else if (command == value_command_os_set_date_time)
    {
        //datetime
        //option_command_os_datetime
    }
    else if (command == value_command_os_bootloader_info)
    {
        //query
        //option_command_os_query
    }

    return EXIT_CODE_SUCCESS;
}

#if 0
int command_processor::run_group_settings(QCommandLineParser *parser, QString command)
{
}

int command_processor::run_group_shell(QCommandLineParser *parser, QString command)
{
}

int command_processor::run_group_stat(QCommandLineParser *parser, QString command)
{
}

int command_processor::run_group_zephyr(QCommandLineParser *parser, QString command)
{
}

int command_processor::run_group_img(QCommandLineParser *parser, QString command)
{
}
#endif

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
    if (error_string != nullptr)
    {
        qDebug() << "status: " << user_data << ", " << status << ", " << error_string;
    }
    else
    {
        qDebug() << "status: " << user_data << ", " << status;
    }
}

void command_processor::progress(uint8_t user_data, uint8_t percent)
{
    qDebug() << "progress: " << user_data << ", " << percent;
}

/******************************************************************************/
// END OF FILE
/******************************************************************************/
