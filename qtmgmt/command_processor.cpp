/******************************************************************************
** Copyright (C) 2024-2025 Jamie M.
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

//Enumeration management group
const QCommandLineOption option_command_enum_index("index", "Index (0-based)", "index");
//const QCommandLineOption option_command_enum_groups("groups", "List of groups (comma separated)", "groups");

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
const QCommandLineOption option_command_img_test("test", "Mark image as test");
const QCommandLineOption option_command_img_confirm("confirm", "Mark image as confirmed");
const QCommandLineOption option_command_img_reset("reset", "Reset after update");
const QCommandLineOption option_command_img_image("image", "Image number", "image");
const QCommandLineOption option_command_img_file("file", "Firmware update", "file");
const QCommandLineOption option_command_img_upgrade("upgrade", "Only accept upgrades");
const QCommandLineOption option_command_img_slot("slot", "Slot number", "slot");

//Shell management group
const QCommandLineOption option_command_shell_run("run", "Command to execute", "command");

//Statistics management group
const QCommandLineOption option_command_stats_group("group", "Group to get statistics of", "group");

//SMP options
const QCommandLineOption option_mtu("mtu", "MTU (default: 256, can be: 96-16384)", "mtu");
const QCommandLineOption option_smp_v1("smp-v1", "Use SMP version 1");
const QCommandLineOption option_smp_v2("smp-v2", "Use SMP version 2 (default)");

const QString indent = "    ";
#ifdef WIN32
const QString newline = "\r\n";
#else
const QString newline = "\n";
#endif

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
    active_group = nullptr;
    active_transport = nullptr;
    mode = ACTION_IDLE;
    smp_v2 = true;
    smp_mtu = 256;
    enum_mgmt_group_ids = nullptr;
    enum_mgmt_group_details = nullptr;
    fs_mgmt_hash_checksum = nullptr;
    fs_mgmt_supported_hashes_checksums = nullptr;
    os_mgmt_os_application_info_response = nullptr;
    os_mgmt_bootloader_info_response = nullptr;
    os_mgmt_task_list = nullptr;
    os_mgmt_memory_pool = nullptr;
    img_mgmt_get_state_images = nullptr;
    img_mgmt_slot_info_images = nullptr;
    stat_mgmt_stats = nullptr;
    stat_mgmt_groups = nullptr;
    is_interactive_mode = false;

    //Execute run function in event loop so that QCoreApplication::exit() works
    QTimer::singleShot(0, this, SLOT(run()));
}

command_processor::~command_processor()
{
#if 0
    if (transport_uart != nullptr)
    {
        if (transport_uart->is_connected() == 1)
        {
            transport_uart->disconnect(true);
        }

        delete transport_uart;
        transport_uart = nullptr;
    }
#endif

    if (enum_mgmt_group_ids != nullptr)
    {
        delete enum_mgmt_group_ids;
        enum_mgmt_group_ids = nullptr;
    }

    if (enum_mgmt_group_details != nullptr)
    {
        delete enum_mgmt_group_details;
        enum_mgmt_group_details = nullptr;
    }

    if (fs_mgmt_hash_checksum != nullptr)
    {
        delete fs_mgmt_hash_checksum;
        fs_mgmt_hash_checksum = nullptr;
    }

    if (fs_mgmt_supported_hashes_checksums != nullptr)
    {
        delete fs_mgmt_supported_hashes_checksums;
        fs_mgmt_supported_hashes_checksums = nullptr;
    }

    if (os_mgmt_os_application_info_response != nullptr)
    {
        delete os_mgmt_os_application_info_response;
        os_mgmt_os_application_info_response = nullptr;
    }

    if (os_mgmt_bootloader_info_response != nullptr)
    {
        delete os_mgmt_bootloader_info_response;
        os_mgmt_bootloader_info_response = nullptr;
    }

    if (os_mgmt_task_list != nullptr)
    {
        delete os_mgmt_task_list;
        os_mgmt_task_list = nullptr;
    }

    if (os_mgmt_memory_pool != nullptr)
    {
        delete os_mgmt_memory_pool;
        os_mgmt_memory_pool = nullptr;
    }

    if (img_mgmt_get_state_images != nullptr)
    {
        delete img_mgmt_get_state_images;
        img_mgmt_get_state_images = nullptr;
    }

    if (img_mgmt_slot_info_images != nullptr)
    {
        delete img_mgmt_slot_info_images;
        img_mgmt_slot_info_images = nullptr;
    }

    if (stat_mgmt_stats != nullptr)
    {
        delete stat_mgmt_stats;
        stat_mgmt_stats = nullptr;
    }

    if (stat_mgmt_groups != nullptr)
    {
        delete stat_mgmt_groups;
        stat_mgmt_groups = nullptr;
    }

    if (active_group != nullptr)
    {
        disconnect(active_group, SIGNAL(status(uint8_t,group_status,QString)), this, SLOT(status(uint8_t,group_status,QString)));
        disconnect(active_group, SIGNAL(progress(uint8_t,uint8_t)), this, SLOT(progress(uint8_t,uint8_t)));
        active_group = nullptr;
    }

    if (active_transport != nullptr)
    {
        disconnect(active_transport, SIGNAL(connected()), this, SLOT(transport_connected()));
        disconnect(active_transport, SIGNAL(disconnected()), this, SLOT(transport_disconnected()));
        disconnect(active_transport, SIGNAL(receive_waiting(smp_message*)), processor, SLOT(message_received(smp_message*)));

        if (active_transport->is_connected() == 1)
        {
            active_transport->disconnect(true);
        }

        delete active_transport;
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

void command_processor::run(QStringList args)
{
    QCommandLineParser parser;
    QList<entry_t> entries;
    const QCommandLineOption option_transport("transport", "MCUmgr transport", "type");
    const QCommandLineOption option_group("group", "MCUmgr group", "type");
    const QCommandLineOption option_command("command", "MCUmgr group command", "command");
    const QCommandLineOption option_help(QStringList() << "h" << "help", "Show contextual help for provided options");
    const QCommandLineOption option_help_all("help-all", "Show all help of every possible option");
    const QCommandLineOption option_help_transports("help-transports", "Show all help of all transports");
    const QCommandLineOption option_help_groups("help-groups", "Show all help of all groups");
    const QCommandLineOption option_help_commands("help-commands", "Show all help of all groups");
    const QCommandLineOption option_interactive("interactive", "Interactive mode");
    const QCommandLineOption option_version = parser.addVersionOption();
    const QCommandLineOption option_quit("quit", "Quit");
    int exit_code = EXIT_CODE_SUCCESS;
    QString user_transport;
    QString user_group;
    QString user_command;
    uint8_t i;
    uint8_t l;
    bool failed = false;
    QEventLoop wait_loop;
    uint16_t active_transport_index = 0;
    uint16_t active_group_index = 0;
    uint16_t active_command_index = 0;

    parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
    parser.addOption(option_help);
    parser.addOption(option_help_all);
    parser.addOption(option_help_transports);
    parser.addOption(option_help_groups);
    parser.addOption(option_help_commands);
    parser.addOption(option_transport);
    parser.addOption(option_group);
    parser.addOption(option_command);
    parser.addOption(option_mtu);

    if (is_interactive_mode == true)
    {
        parser.addOption(option_quit);
    }
    else
    {
        parser.addOption(option_interactive);
    }

    parser.parse(args);

    if (is_interactive_mode == true && parser.isSet(option_quit))
    {
        text_thread_object.set_quit();
        text_thread_wait_condition.wakeAll();
        text_thread_object.wait(1000);
        return QCoreApplication::exit(0);
    }

    if (parser.isSet(option_version))
    {
        fputs(qPrintable(QCoreApplication::applicationName() % tr(" version ") % QCoreApplication::applicationVersion() % newline), stdout);
        return return_status(EXIT_CODE_SUCCESS);
    }

    if (((parser.isSet(option_help_all) ? 1 : 0) + (parser.isSet(option_help_transports) ? 1 : 0) + (parser.isSet(option_help_groups) ? 1 : 0) + (parser.isSet(option_help_commands) ? 1 : 0) + (parser.isSet(option_help) ? 1 : 0)) > 1)
    {
        QString help_options = "--" % option_help.names().join(" or --") % " or --" % option_help_all.names().join(" or --") % " or --" % option_help_transports.names().join(" or --") % " or --" % option_help_groups.names().join(" or --") % " or --" % option_help_commands.names().join(" or --");
        fputs(qPrintable(tr("Conflicting command line options, only one of: ") % help_options % tr(" may be provided") % newline), stdout);
        return return_status(EXIT_CODE_SUCCESS);
    }

    //Check for special help options
    if (parser.isSet(option_help_all) || parser.isSet(option_help_transports) || parser.isSet(option_help_groups) || parser.isSet(option_help_commands))
    {
        i = 0;

        if (parser.isSet(option_help_all))
        {
            fputs(qPrintable("Not yet supported\n"), stdout);
        }
        else if (parser.isSet(option_help_transports))
        {
            l = supported_transports.length();

            fputs(qPrintable(tr("Supported transports:") % newline), stdout);

            while (i < l)
            {
                uint8_t i2 = 0;
                uint8_t l2;
                QList<entry_t> transport_entry;

                (this->*supported_transports[i].options_function)(&transport_entry);
                fputs(qPrintable(indent % supported_transports[i].name % ":" % newline % indent % indent % "--transport " % supported_transports[i].arguments.join(" or --transport ") % newline), stdout);
                l2 = transport_entry.length();

                while (i2 < l2)
                {
                    QString argument_text;
                    uint8_t i3 = 0;
                    uint8_t l3 = transport_entry[i2].option.length();

                    while (i3 < l3)
                    {
                        uint8_t i4 = 0;
                        uint8_t l4 = transport_entry[i2].option[i3]->names().length();

                        while (i4 < l4)
                        {
                            argument_text.append("--" % transport_entry[i2].option[i4]->names()[i4]);

                            if (transport_entry[i2].option[i3]->valueName().length() > 0)
                            {
                                argument_text.append(" <" % transport_entry[i2].option[i3]->valueName() % ">");
                            }

                            argument_text.append(" (" % transport_entry[i2].option[i3]->description() % ")");
                            ++i4;

                            if (i4 < l4)
                            {
                                argument_text.append(" or ");
                            }
                        }

                        ++i3;

                        if (i3 < l3)
                        {
                            argument_text.append((transport_entry[i2].exclusive == true ? tr(" or ") : tr(" and ")));
                        }
                    }

                    fputs(qPrintable(indent % indent % indent % argument_text % newline), stdout);
                    ++i2;
                }

                ++i;
            }
        }
        else if (parser.isSet(option_help_groups))
        {
            uint8_t l = supported_groups.length();

            fputs(qPrintable(tr("Supported groups:") % newline), stdout);

            while (i < l)
            {
                fputs(qPrintable(indent % supported_groups[i].name % ":" % newline % indent % indent % "--group " % supported_groups[i].arguments.join(" or --group ") % newline), stdout);
                ++i;
            }
        }
        else if (parser.isSet(option_help_commands))
        {
            uint8_t l = supported_groups.length();

            fputs(qPrintable(tr("Supported commands:") % newline), stdout);

            while (i < l)
            {
                uint8_t i2 = 0;
                uint8_t l2 = supported_groups[i].commands.length();

                fputs(qPrintable(indent % supported_groups[i].name % ":" % newline % indent % indent % "--group " % supported_groups[i].arguments.join(" or --group ") % newline), stdout);

                while (i2 < l2)
                {
                    uint8_t i3 = 0;
                    uint8_t l3;
                    QList<entry_t> command_entry;

                    if (supported_groups[i].commands[i2].add_function != nullptr)
                    {
                        (this->*supported_groups[i].commands[i2].add_function)(&command_entry);
                    }

                    fputs(qPrintable(indent % indent % indent % supported_groups[i].commands[i2].name % ":" % newline % indent % indent % indent % indent % "--command " % supported_groups[i].commands[i2].arguments.join(" or --command ") % newline), stdout);
                    l3 = command_entry.length();

                    while (i3 < l3)
                    {
                        QString argument_text;
                        uint8_t i4 = 0;
                        uint8_t l4 = command_entry[i3].option.length();

                        while (i4 < l4)
                        {
                            uint8_t i5 = 0;
                            uint8_t l5 = command_entry[i3].option[i4]->names().length();

                            while (i5 < l5)
                            {
                                argument_text.append("--" % command_entry[i3].option[i4]->names()[i5]);

                                if (command_entry[i3].option[i4]->valueName().length() > 0)
                                {
                                    argument_text.append(" <" % command_entry[i3].option[i4]->valueName() % ">");
                                }

                                argument_text.append(" (" % command_entry[i3].option[i4]->description() % ")");
                                ++i5;

                                if (i5 < l5)
                                {
                                    argument_text.append(" or ");
                                }
                            }

                            ++i4;

                            if (i4 < l4)
                            {
                                argument_text.append((command_entry[i3].exclusive == true ? tr(" or ") : tr(" and ")));
                            }
                        }

                        fputs(qPrintable(indent % indent % indent % indent % indent % argument_text % newline), stdout);
                        ++i3;
                    }

                    ++i2;
                }

                ++i;
            }
        }

        return return_status(EXIT_CODE_SUCCESS);
    }

    //Add SMP version command line
    entries.append({{&option_smp_v1, &option_smp_v2}, false, true});

    if (parser.isSet(option_transport))
    {
        i = 0;
        l = supported_transports.length();

        user_transport = parser.value(option_transport);

        while (i < l)
        {
            if (supported_transports[i].arguments.indexOf(user_transport) != -1)
            {
                (this->*supported_transports[i].options_function)(&entries);
                active_transport_index = i;
                break;
            }

            ++i;
        }

        if (i == l)
        {
            fputs(qPrintable(tr("Error: invalid transport specified")), stdout);
            return return_status(EXIT_CODE_INVALID_TRANSPORT);
        }
    }

    if (parser.isSet(option_group))
    {
        i = 0;
        l = supported_groups.length();

        user_group = parser.value(option_group);

        while (i < l)
        {
            if (supported_groups[i].arguments.indexOf(user_group) != -1)
            {
                if (parser.isSet(option_command))
                {
                    uint8_t i2 = 0;
                    uint8_t l2 = supported_groups[i].commands.length();

                    user_command = parser.value(option_command);

                    while (i2 < l2)
                    {
                        if (supported_groups[i].commands[i2].arguments.indexOf(user_command) != -1)
                        {
                            if (supported_groups[i].commands[i2].add_function != nullptr)
                            {
                                (this->*supported_groups[i].commands[i2].add_function)(&entries);
                            }

                            active_command_index = i2;
                            break;
                        }

                        ++i2;
                    }

                    if (i2 == l2)
                    {
                        fputs(qPrintable(tr("Error: invalid command specified")), stdout);
                        return return_status(EXIT_CODE_INVALID_COMMAND);
                    }
                }

                active_group_index = i;
                break;
            }

            ++i;
        }

        if (i == l)
        {
            fputs(qPrintable(tr("Error: invalid group specified")), stdout);
            return return_status(EXIT_CODE_INVALID_GROUP);
        }
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
        return return_status(EXIT_CODE_SUCCESS);
    }

    if (is_interactive_mode == false && parser.isSet(option_interactive))
    {
        return interactive_mode();
    }

    if (!parser.isSet(option_transport) || !parser.isSet(option_group) || !parser.isSet(option_command))
    {
        if (!parser.isSet(option_transport))
        {
            fputs(qPrintable(tr("Missing required argument: ") % "--" % option_transport.names().join(" or --") % newline), stdout);
        }

        if (!parser.isSet(option_group))
        {
            fputs(qPrintable(tr("Missing required argument: ") % "--" % option_group.names().join(" or --") % newline), stdout);
        }

        if (!parser.isSet(option_command))
        {
            fputs(qPrintable(tr("Missing required argument: ") % "--" % option_command.names().join(" or --") % newline), stdout);
        }

        return return_status(EXIT_CODE_MISSING_REQUIRED_ARGUMENTS);
    }

    //Re-parse command line arguments after transport/group/command options have been added
    parser.parse(args);

    if (parser.unknownOptionNames().length() > 0 || parser.positionalArguments().length() > 0)
    {
        fputs(qPrintable(tr("Unknown arguments provided: ") % (QStringList() << parser.unknownOptionNames() << parser.positionalArguments()).join(", ")), stdout);
        return return_status(EXIT_CODE_UNKNOWN_ARGUMENTS_PROVIDED);
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
            uint8_t i2 = 0;
            uint8_t l2 = entries[i].option.length();

            while (i2 < l2)
            {
                if (parser.isSet(*entries[i].option[i2]) == true)
                {
                    if (entries[i].exclusive == true && option_present == true)
                    {
                        //We already have an exclusive option so this configuration is not allowed
                        exclusivity_breached = true;
                        break;
                    }

                    option_present = true;
                }

                ++i2;
            }

            if (entries[i].required == true && option_present == false)
            {
                //Required option not present, output error with list of options
                QString required_options;

                i2 = 0;
                l2 = entries[i].option.length();

                while (i2 < l2)
                {
                    required_options.append(entries[i].option[i2]->names().join(" or --"));
                    ++i2;

                    if (i2 < l2)
                    {
                        required_options.append(" or --");
                    }
                }

                fputs(qPrintable(tr("Missing required argument: ") % "--" % required_options % newline), stdout);
                failed = true;
            }
            else if (exclusivity_breached == true)
            {
                //Multiple exclusive options used, output error with details
                QString conflicting_options;

                i2 = 0;
                l2 = entries[i].option.length();

                while (i2 < l2)
                {
                    if (parser.isSet(*entries[i].option[i2]))
                    {
                        conflicting_options.append(entries[i].option[i2]->names().join(" and --")).append(" and --");
                    }

                    ++i2;
                }

                conflicting_options.remove((conflicting_options.length() - 7), 7);
                fputs(qPrintable(tr("Conflicting exclusive arguments: ") % "--" % conflicting_options % newline), stdout);
                failed = true;
            }
        }

        ++i;
    }

    if (failed == true)
    {
        return return_status(EXIT_CODE_MISSING_REQUIRED_ARGUMENTS);
    }

//TODO: Check that options supplied for each transport/group are valid

    //Apply SMP parameters
    if (parser.isSet(option_mtu) == true)
    {
        smp_mtu = parser.value(option_mtu).toUInt();

        if (smp_mtu < minimum_smp_mtu || smp_mtu > maximum_smp_mtu)
        {
            fputs(qPrintable(tr("Argument out of range: ") % "--" % option_mtu.names().first() % newline), stdout);
            return return_status(EXIT_CODE_NUMERIAL_ARGUMENT_OUT_OF_RANGE);
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
    else if (user_transport == value_transport_uart || user_transport == value_transport_serial)
    {
        transport_uart = new smp_uart(this);
        active_transport = transport_uart;
    }
#endif
#if defined(PLUGIN_MCUMGR_TRANSPORT_BLUETOOTH)
    else if (user_transport == value_transport_bluetooth || user_transport == value_transport_bt)
    {
        transport_bluetooth = new smp_bluetooth(this);
        active_transport = transport_bluetooth;
    }
#endif
#if defined(PLUGIN_MCUMGR_TRANSPORT_UDP)
    else if (user_transport == value_transport_udp)
    {
        transport_udp = new smp_udp(this);
        active_transport = transport_udp;
    }
#endif
#if defined(PLUGIN_MCUMGR_TRANSPORT_LORAWAN)
    else if (user_transport == value_transport_lorawan)
    {
        transport_lorawan = new smp_lorawan(this);
        active_transport = transport_lorawan;
    }
#endif

    exit_code = (this->*supported_transports[active_transport_index].configure_function)(active_transport, &parser);

    if (exit_code != EXIT_CODE_SUCCESS)
    {
        return return_status(exit_code);
    }

    //Open transport, exit if it failed
    connect(active_transport, SIGNAL(connected()), this, SLOT(transport_connected()));
    connect(active_transport, SIGNAL(disconnected()), this, SLOT(transport_disconnected()));
    connect(active_transport, SIGNAL(connected()), &wait_loop, SLOT(quit()));
    exit_code = active_transport->connect();

    if (exit_code != SMP_TRANSPORT_ERROR_OK)
    {
        fputs(qPrintable(tr("Transport open failed: ") % QString::number(exit_code) % newline), stdout);
        return return_status(EXIT_CODE_TRANSPORT_OPEN_FAILED);
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

    switch (supported_groups[active_group_index].group_id)
    {
        case SMP_GROUP_ID_ENUM:
        {
            group_enum = new smp_group_enum_mgmt(processor);
            active_group = group_enum;
            break;
        }
        case SMP_GROUP_ID_FS:
        {
            group_fs = new smp_group_fs_mgmt(processor);
            active_group = group_fs;
            break;
        }
        case SMP_GROUP_ID_IMG:
        {
            group_img = new smp_group_img_mgmt(processor);
            active_group = group_img;
            break;
        }
        case SMP_GROUP_ID_OS:
        {
            group_os = new smp_group_os_mgmt(processor);
            active_group = group_os;
            break;
        }
        case SMP_GROUP_ID_SETTINGS:
        {
            group_settings = new smp_group_settings_mgmt(processor);
            active_group = group_settings;
            break;
        }
        case SMP_GROUP_ID_SHELL:
        {
            group_shell = new smp_group_shell_mgmt(processor);
            active_group = group_shell;
            break;
        }
        case SMP_GROUP_ID_STATS:
        {
            group_stat = new smp_group_stat_mgmt(processor);
            active_group = group_stat;
            break;
        }
        case SMP_GROUP_ID_ZEPHYR:
        {
            group_zephyr = new smp_group_zephyr_mgmt(processor);
            active_group = group_zephyr;
            break;
        }
        default:
        {
            break;
        }
    };

    connect(active_group, SIGNAL(status(uint8_t,group_status,QString)), this, SLOT(status(uint8_t,group_status,QString)));
    connect(active_group, SIGNAL(progress(uint8_t,uint8_t)), this, SLOT(progress(uint8_t,uint8_t)));

    processor->set_transport(active_transport);

    exit_code = (this->*supported_groups[active_group_index].commands[active_command_index].run_function)(&parser);

    if (exit_code != EXIT_CODE_SUCCESS)
    {
        return return_status(exit_code);
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

int command_processor::configure_transport_options_uart(smp_transport *transport, QCommandLineParser *parser)
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
        uart_configuration.baud = default_transport_uart_baud;
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

    static_cast<smp_uart *>(transport)->set_connection_config(&uart_configuration);
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

int command_processor::configure_transport_options_bluetooth(smp_transport *transport, QCommandLineParser *parser)
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

    static_cast<smp_bluetooth *>(transport)->set_connection_config(&bluetooth_configuration);
    return EXIT_CODE_SUCCESS;
}
#endif

#if defined(PLUGIN_MCUMGR_TRANSPORT_UDP)
void command_processor::add_transport_options_udp(QList<entry_t> *entries)
{
    entries->append({{&option_transport_udp_host}, true, false});
    entries->append({{&option_transport_udp_port}, false, false});
}

int command_processor::configure_transport_options_udp(smp_transport *transport, QCommandLineParser *parser)
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
        udp_configuration.port = default_transport_udp_port;
    }

    static_cast<smp_udp *>(transport)->set_connection_config(&udp_configuration);
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

int command_processor::configure_transport_options_lorawan(smp_transport *transport, QCommandLineParser *parser)
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
            lorawan_configuration.port = default_transport_lorawan_port_ssl;
        }
        else
        {
            lorawan_configuration.port = default_transport_lorawan_port;
        }
    }

    converted = false;

    lorawan_configuration.frame_port = parser->value(option_transport_lorawan_frame_port).toUInt(&converted);

    if (converted == false)
    {
        return EXIT_CODE_NUMERIAL_ARGUMENT_CONVERSION_FAILED;
    }

    static_cast<smp_lorawan *>(transport)->set_connection_config(&lorawan_configuration);
    return EXIT_CODE_SUCCESS;
}
#endif

//HERE
int command_processor::run_group_enum_command_count(QCommandLineParser *parser)
{
    mode = ACTION_ENUM_COUNT;
    set_group_transport_settings(active_group);

    if (group_enum->start_enum_count(&enum_mgmt_count) == true)
    {
        return EXIT_CODE_SUCCESS;
    }

    return EXIT_CODE_TODO_AA;
}

int command_processor::run_group_enum_command_list(QCommandLineParser *parser)
{
    enum_mgmt_group_ids = new QList<uint16_t>();
    mode = ACTION_ENUM_LIST;
    set_group_transport_settings(active_group);

    if (group_enum->start_enum_list(enum_mgmt_group_ids) == true)
    {
        return EXIT_CODE_SUCCESS;
    }

    return EXIT_CODE_TODO_AA;
}

void command_processor::add_group_enum_command_single(QList<entry_t> *entries)
{
    //index
    entries->append({{&option_command_enum_index}, true, false});
}

int command_processor::run_group_enum_command_single(QCommandLineParser *parser)
{
    mode = ACTION_ENUM_SINGLE;
    set_group_transport_settings(active_group);

    if (group_enum->start_enum_single(parser->value(option_command_enum_index).toUInt(), &enum_mgmt_id, &enum_mgmt_end) == true)
    {
        return EXIT_CODE_SUCCESS;
    }

    return EXIT_CODE_TODO_AA;
}

//void command_processor::add_group_enum_command_details(QList<entry_t> *entries)
//{
//groups (array)
//entries->append({{&option_command_enum_groups}, true, false});
//const QCommandLineOption option_command_enum_groups("groups", "List of groups (comma separated)", "groups");
//}

int command_processor::run_group_enum_command_details(QCommandLineParser *parser)
{
    enum_mgmt_group_details = new QList<enum_details_t>;
    mode = ACTION_ENUM_DETAILS;
    set_group_transport_settings(active_group);

    if (group_enum->start_enum_details(enum_mgmt_group_details, &enum_mgmt_group_fields_present) == true)
    {
        return EXIT_CODE_SUCCESS;
    }

    return EXIT_CODE_TODO_AA;
}

//WORKING
void command_processor::add_group_fs_command_upload_download(QList<entry_t> *entries)
{
    //local file, remote file
    entries->append({{&option_command_fs_local_file}, true, false});
    entries->append({{&option_command_fs_remote_file}, true, false});
}

int command_processor::run_group_fs_command_upload(QCommandLineParser *parser)
{
    //TODO
    mode = ACTION_FS_UPLOAD;
    set_group_transport_settings(active_group);

    if (group_fs->start_upload(parser->value(option_command_fs_local_file), parser->value(option_command_fs_remote_file)) == true)
    {
        return EXIT_CODE_SUCCESS;
    }

    return EXIT_CODE_TODO_AA;
}

int command_processor::run_group_fs_command_download(QCommandLineParser *parser)
{
    //TODO
    mode = ACTION_FS_DOWNLOAD;
    set_group_transport_settings(active_group);

    if (group_fs->start_download(parser->value(option_command_fs_remote_file), parser->value(option_command_fs_local_file)) == true)
    {
        return EXIT_CODE_SUCCESS;
    }

    return EXIT_CODE_TODO_AA;
}

void command_processor::add_group_fs_command_status(QList<entry_t> *entries)
{
    //remote file
    entries->append({{&option_command_fs_remote_file}, true, false});
}

int command_processor::run_group_fs_command_status(QCommandLineParser *parser)
{
    //TODO
    mode = ACTION_FS_STATUS;
    set_group_transport_settings(active_group);

    if (group_fs->start_status(parser->value(option_command_fs_remote_file), &fs_mgmt_file_size) == true)
    {
        return EXIT_CODE_SUCCESS;
    }

    return EXIT_CODE_TODO_AA;
}

void command_processor::add_group_fs_command_hash_checksum(QList<entry_t> *entries)
{
    //remote file, hash/checksum
    entries->append({{&option_command_fs_remote_file}, true, false});
//TODO: optional?
    entries->append({{&option_command_fs_hash_checksum}, true, false});
}

int command_processor::run_group_fs_command_hash_checksum(QCommandLineParser *parser)
{
    //TODO
    fs_mgmt_hash_checksum = new QByteArray();
    mode = ACTION_FS_HASH_CHECKSUM;
    set_group_transport_settings(active_group);

//TODO: optional?
    if (group_fs->start_hash_checksum(parser->value(option_command_fs_remote_file), parser->value(option_command_fs_hash_checksum), fs_mgmt_hash_checksum, &fs_mgmt_file_size) == true)
    {
        return EXIT_CODE_SUCCESS;
    }

    return EXIT_CODE_TODO_AA;
}

int command_processor::run_group_fs_command_supported_hashes_checksums(QCommandLineParser *parser)
{
    //TODO
    fs_mgmt_supported_hashes_checksums = new QList<hash_checksum_t>();
    mode = ACTION_FS_SUPPORTED_HASHES_CHECKSUMS;
    set_group_transport_settings(active_group);

    if (group_fs->start_supported_hashes_checksums(fs_mgmt_supported_hashes_checksums) == true)
    {
        return EXIT_CODE_SUCCESS;
    }

    return EXIT_CODE_TODO_AA;
}

int command_processor::run_group_fs_command_close_file(QCommandLineParser *parser)
{
    mode = ACTION_FS_CLOSE_FILE;
    set_group_transport_settings(active_group);

    if (group_fs->start_file_close() == true)
    {
        return EXIT_CODE_SUCCESS;
    }

    return EXIT_CODE_TODO_AA;
}

int command_processor::run_group_img_command_get_state(QCommandLineParser *parser)
{
    img_mgmt_get_state_images = new QList<image_state_t>();
    mode = ACTION_IMG_IMAGE_LIST;
    set_group_transport_settings(active_group);

    if (group_img->start_image_get(img_mgmt_get_state_images) == true)
    {
        return EXIT_CODE_SUCCESS;
    }

    return EXIT_CODE_TODO_AA;
}

void command_processor::add_group_img_command_set_state(QList<entry_t> *entries)
{
    //hash, confirm
    //TODO: need to check if supplied parameters are valid
    entries->append({{&option_command_img_hash}, false, false});
    entries->append({{&option_command_img_confirm}, false, false});
}

int command_processor::run_group_img_command_set_state(QCommandLineParser *parser)
{
    QByteArray hash;

    if (parser->isSet(option_command_img_hash))
    {
        hash = QByteArray::fromHex(parser->value(option_command_img_hash).toLatin1());
    }

    mode = ACTION_IMG_IMAGE_SET;
    set_group_transport_settings(active_group);

    if (group_img->start_image_set((parser->isSet(option_command_img_hash) ? &hash : nullptr), (parser->isSet(option_command_img_confirm) ? true : false), nullptr) == true)
    {
        return EXIT_CODE_SUCCESS;
    }

    return EXIT_CODE_TODO_AA;
}

void command_processor::add_group_img_command_upload(QList<entry_t> *entries)
{
    //image, file, upgrade, test/confirm, reset
    entries->append({{&option_command_img_image}, false, false});
    entries->append({{&option_command_img_file}, true, false});
    entries->append({{&option_command_img_upgrade}, false, false});
    entries->append({{&option_command_img_test, &option_command_img_confirm}, false, true});
    entries->append({{&option_command_img_reset}, false, false});
}

int command_processor::run_group_img_command_upload(QCommandLineParser *parser)
{
    mode = ACTION_IMG_UPLOAD;
    upload_mode = (parser->isSet(option_command_img_test) == true ? IMAGE_UPLOAD_MODE_TEST : (parser->isSet(option_command_img_confirm) == true ? IMAGE_UPLOAD_MODE_CONFIRM : IMAGE_UPLOAD_MODE_NORMAL));
    upload_reset = parser->isSet(option_command_img_reset);
    set_group_transport_settings(active_group);

    if (group_img->start_firmware_update((parser->isSet(option_command_img_image) ? parser->value(option_command_img_image).toUInt() : 0), parser->value(option_command_img_file), (parser->isSet(option_command_img_upgrade) ? true : false), &upload_hash) == true)
    {
        return EXIT_CODE_SUCCESS;
    }

    return EXIT_CODE_TODO_AA;
}

void command_processor::add_group_img_command_erase_slot(QList<entry_t> *entries)
{
    //slot
    entries->append({{&option_command_img_slot}, true, false});
}

int command_processor::run_group_img_command_erase_slot(QCommandLineParser *parser)
{
    mode = ACTION_IMG_IMAGE_ERASE;
    set_group_transport_settings(active_group);

    if (group_img->start_image_erase(parser->value(option_command_img_slot).toUInt()) == true)
    {
        return EXIT_CODE_SUCCESS;
    }

    return EXIT_CODE_TODO_AA;
}

int command_processor::run_group_img_command_slot_info(QCommandLineParser *parser)
{
    //TODO
    img_mgmt_slot_info_images = new QList<slot_info_t>();
    mode = ACTION_IMG_IMAGE_SLOT_INFO;
    set_group_transport_settings(active_group);

    if (group_img->start_image_slot_info(img_mgmt_slot_info_images) == true)
    {
        return EXIT_CODE_SUCCESS;
    }

    return EXIT_CODE_TODO_AA;
}

void command_processor::add_group_os_command_echo(QList<entry_t> *entries)
{
    //data
    entries->append({{&option_command_os_data}, true, false});
}

int command_processor::run_group_os_command_echo(QCommandLineParser *parser)
{
    //data
    mode = ACTION_OS_ECHO;
    set_group_transport_settings(active_group);

    if (group_os->start_echo(parser->value(option_command_os_data)) == true)
    {
        return EXIT_CODE_SUCCESS;
    }

    return EXIT_CODE_TODO_AA;
}

int command_processor::run_group_os_command_task_list(QCommandLineParser *parser)
{
    //TODO
    os_mgmt_task_list = new QList<task_list_t>();
    mode = ACTION_OS_TASK_STATS;
    set_group_transport_settings(active_group);

    if (group_os->start_task_stats(os_mgmt_task_list) == true)
    {
        return EXIT_CODE_SUCCESS;
    }

    return EXIT_CODE_TODO_AA;
}

int command_processor::run_group_os_command_memory_pool(QCommandLineParser *parser)
{
    //TODO
    os_mgmt_memory_pool = new QList<memory_pool_t>();
    mode = ACTION_OS_MEMORY_POOL;
    set_group_transport_settings(active_group);

    if (group_os->start_memory_pool(os_mgmt_memory_pool) == true)
    {
        return EXIT_CODE_SUCCESS;
    }

    return EXIT_CODE_TODO_AA;
}

void command_processor::add_group_os_command_reset(QList<entry_t> *entries)
{
    //force
    entries->append({{&option_command_os_force}, false, false});
}

int command_processor::run_group_os_command_reset(QCommandLineParser *parser)
{
    //force
    mode = ACTION_OS_RESET;
    set_group_transport_settings(active_group);

    if (group_os->start_reset(parser->isSet(option_command_os_force)) == true)
    {
        return EXIT_CODE_SUCCESS;
    }

    return EXIT_CODE_TODO_AA;
}

int command_processor::run_group_os_command_mcumgr_parameters(QCommandLineParser *parser)
{
    mode = ACTION_OS_MCUMGR_BUFFER;
    set_group_transport_settings(active_group);

    if (group_os->start_mcumgr_parameters(&os_mgmt_mcumgr_parameters_buffer_size, &os_mgmt_mcumgr_parameters_buffer_count) == true)
    {
        return EXIT_CODE_SUCCESS;
    }
}

void command_processor::add_group_os_command_application_information(QList<entry_t> *entries)
{
    //format
    entries->append({{&option_command_os_format}, false, false});
}

int command_processor::run_group_os_command_application_information(QCommandLineParser *parser)
{
    //format
    os_mgmt_os_application_info_response = new QString();
    mode = ACTION_OS_OS_APPLICATION_INFO;
    set_group_transport_settings(active_group);

    if (group_os->start_os_application_info((parser->isSet(option_command_os_format) == true ? parser->value(option_command_os_format) : ""), os_mgmt_os_application_info_response) == true)
    {
        return EXIT_CODE_SUCCESS;
    }

    return EXIT_CODE_TODO_AA;
}

int command_processor::run_group_os_command_get_time_and_date(QCommandLineParser *parser)
{
    //TODO
    mode = ACTION_OS_DATETIME_GET;
    set_group_transport_settings(active_group);

    //if (group_os->start_date_time_get() == true)
    //{
    //    return EXIT_CODE_SUCCESS;
    //}

    return EXIT_CODE_TODO_AA;
    //QDateTime *date_time
}

void command_processor::add_group_os_command_set_time_and_date(QList<entry_t> *entries)
{
    //datetime
    entries->append({{&option_command_os_datetime}, true, false});
}

int command_processor::run_group_os_command_set_time_and_date(QCommandLineParser *parser)
{
    //TODO
    //datetime
    //option_command_os_datetime
    mode = ACTION_OS_DATETIME_SET;
    set_group_transport_settings(active_group);

    //if (group_os->start_date_time_set() == true)
    //{
    //    return EXIT_CODE_SUCCESS;
    //}

    return EXIT_CODE_TODO_AA;
}

void command_processor::add_group_os_command_bootloader_information(QList<entry_t> *entries)
{
    //query
    entries->append({{&option_command_os_query}, false, false});
}

int command_processor::run_group_os_command_bootloader_information(QCommandLineParser *parser)
{
    //query
    os_mgmt_bootloader_info_response = new QVariant();
    mode = ACTION_OS_BOOTLOADER_INFO;
    set_group_transport_settings(active_group);

    if (group_os->start_bootloader_info((parser->isSet(option_command_os_query) == true ? parser->value(option_command_os_query) : ""), os_mgmt_bootloader_info_response) == true)
    {
        return EXIT_CODE_SUCCESS;
    }

    return EXIT_CODE_TODO_AA;
}

void command_processor::add_group_shell_command_execute(QList<entry_t> *entries)
{
    //command
    entries->append({{&option_command_shell_run}, true, false});
}

int command_processor::run_group_shell_command_execute(QCommandLineParser *parser)
{
    QRegularExpression reTempRE("\\s+");
    QStringList list_arguments = parser->value(option_command_shell_run).split(reTempRE);
    mode = ACTION_SHELL_EXECUTE;
    set_group_transport_settings(active_group);

    if (group_shell->start_execute(&list_arguments, &shell_mgmt_rc) == true)
    {
        return EXIT_CODE_SUCCESS;
    }

    return EXIT_CODE_TODO_AA;
}

void command_processor::add_group_stats_command_group_data(QList<entry_t> *entries)
{
    //group
    entries->append({{&option_command_stats_group}, true, false});
}

int command_processor::run_group_stats_command_group_data(QCommandLineParser *parser)
{
    stat_mgmt_stats = new QList<stat_value_t>();
    mode = ACTION_STAT_GROUP_DATA;
    set_group_transport_settings(active_group);

    if (group_stat->start_group_data(parser->value(option_command_shell_run), stat_mgmt_stats) == true)
    {
        return EXIT_CODE_SUCCESS;
    }

    return EXIT_CODE_TODO_AA;
}

int command_processor::run_group_stats_command_list_groups(QCommandLineParser *parser)
{
    stat_mgmt_groups = new QStringList();
    mode = ACTION_STAT_LIST_GROUPS;
    set_group_transport_settings(active_group);

    if (group_stat->start_list_groups(stat_mgmt_groups) == true)
    {
        return EXIT_CODE_SUCCESS;
    }

    return EXIT_CODE_TODO_AA;
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
                if (upload_mode == IMAGE_UPLOAD_MODE_TEST || upload_mode == IMAGE_UPLOAD_MODE_CONFIRM)
                {
                    //Mark image for test or confirmation
                    finished = false;

                    mode = ACTION_IMG_UPLOAD_SET;
                    processor->set_transport(active_transport);
                    set_group_transport_settings(group_img);
                    bool started = group_img->start_image_set(&upload_hash, (upload_mode == IMAGE_UPLOAD_MODE_CONFIRM ? true : false), nullptr);
                    //todo: check status

                    log_debug() << "do upload of " << upload_hash;
                }
            }
            else if (user_data == ACTION_IMG_UPLOAD_SET)
            {
                if (upload_reset == true)
                {
                    //Reboot device
                    finished = false;

                    //Clean up of previous group
                    disconnect(active_group, SIGNAL(status(uint8_t,group_status,QString)), this, SLOT(status(uint8_t,group_status,QString)));
                    disconnect(active_group, SIGNAL(progress(uint8_t,uint8_t)), this, SLOT(progress(uint8_t,uint8_t)));
                    delete active_group;
                    active_group = nullptr;

                    //Set up OS management group
                    group_os = new smp_group_os_mgmt(processor);
                    active_group = group_os;

                    connect(group_os, SIGNAL(status(uint8_t,group_status,QString)), this, SLOT(status(uint8_t,group_status,QString)));
                    connect(group_os, SIGNAL(progress(uint8_t,uint8_t)), this, SLOT(progress(uint8_t,uint8_t)));

                    mode = ACTION_OS_UPLOAD_RESET;
                    processor->set_transport(active_transport);
                    set_group_transport_settings(group_os);

                    bool started = group_os->start_reset(false);
                    //todo: check status

                    log_debug() << "do reset";
                }
            }
            else if (user_data == ACTION_IMG_IMAGE_LIST || user_data == ACTION_IMG_IMAGE_SET)
            {
                uint8_t i = 0;
                uint8_t l = (*img_mgmt_get_state_images).length();

                while (i < l)
                {
                    uint8_t c = 0;
                    uint8_t m = (*img_mgmt_get_state_images)[i].slot_list.length();

                    if ((*img_mgmt_get_state_images)[i].image_set == true)
                    {
                        fputs(qPrintable(tr("Image ") % QString::number((*img_mgmt_get_state_images)[i].image) % newline), stdout);
                    }
                    else
                    {
                        fputs(qPrintable(tr("Image (assumed ") % QString::number(i) % ")" % newline), stdout);
                    }

                    while (c < m)
                    {
                        AutEscape::to_hex(&(*img_mgmt_get_state_images)[i].slot_list[c].hash);
                        fputs(qPrintable(indent % tr("Slot ") % QString::number((*img_mgmt_get_state_images)[i].slot_list[c].slot) % newline), stdout);
                        fputs(qPrintable(indent % indent % tr("Hash: ") % (*img_mgmt_get_state_images)[i].slot_list[c].hash % newline), stdout);
                        fputs(qPrintable(indent % indent % tr("Version: ") % (*img_mgmt_get_state_images)[i].slot_list[c].version % newline), stdout);

                        if ((*img_mgmt_get_state_images)[i].slot_list[c].active == true)
                        {
                            fputs(qPrintable(indent % indent %tr("- Active") % newline), stdout);
                        }

                        if ((*img_mgmt_get_state_images)[i].slot_list[c].bootable == true)
                        {
                            fputs(qPrintable(indent % indent % tr("- Bootable") % newline), stdout);
                        }

                        if ((*img_mgmt_get_state_images)[i].slot_list[c].confirmed == true)
                        {
                            fputs(qPrintable(indent % indent % tr("- Confirmed") % newline), stdout);
                        }

                        if ((*img_mgmt_get_state_images)[i].slot_list[c].pending == true)
                        {
                            fputs(qPrintable(indent % indent % tr("- Pending") % newline), stdout);
                        }

                        if ((*img_mgmt_get_state_images)[i].slot_list[c].permanent == true)
                        {
                            fputs(qPrintable(indent % indent % tr("- Permanent") % newline), stdout);
                        }

                        if ((*img_mgmt_get_state_images)[i].slot_list[c].splitstatus == true)
                        {
                            fputs(qPrintable(indent % indent % tr("- Split image") % newline), stdout);
                        }

                        ++c;
                    }

                    ++i;
                }
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

                    fputs(qPrintable(tr("Image ") % QString::number((*img_mgmt_slot_info_images)[i].image) % newline), stdout);

                    while (c < m)
                    {
                        fputs(qPrintable(indent % tr("Slot ") % QString::number((*img_mgmt_slot_info_images)[i].slot_data[c].slot) % newline), stdout);

                        if ((*img_mgmt_slot_info_images)[i].slot_data[c].size_present == true)
                        {
                            size_abbreviation((*img_mgmt_slot_info_images)[i].slot_data[c].size, &field_size);
                            fputs(qPrintable(indent % indent % tr("Size: ") % field_size % newline), stdout);
                            field_size.clear();
                        }

                        if ((*img_mgmt_slot_info_images)[i].slot_data[c].upload_image_id_present == true)
                        {
                            fputs(qPrintable(indent % indent % tr("Upload image ID: ") % QString::number((*img_mgmt_slot_info_images)[i].slot_data[c].upload_image_id) % newline), stdout);
                        }

                        ++c;
                    }

                    if ((*img_mgmt_slot_info_images)[i].max_image_size_present == true)
                    {
                        size_abbreviation((*img_mgmt_slot_info_images)[i].max_image_size, &field_size);
                        fputs(qPrintable(indent % tr("Max image size: ") % field_size % newline), stdout);
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
                skip_error_string = true;

                if (upload_reset == true)
                {
                    //Reboot device
                    finished = false;

                    //Clean up of previous group
                    disconnect(active_group, SIGNAL(status(uint8_t,group_status,QString)), this, SLOT(status(uint8_t,group_status,QString)));
                    disconnect(active_group, SIGNAL(progress(uint8_t,uint8_t)), this, SLOT(progress(uint8_t,uint8_t)));
                    delete active_group;
                    active_group = nullptr;

                    //Set up OS management group
                    group_os = new smp_group_os_mgmt(processor);
                    active_group = group_os;

                    connect(group_os, SIGNAL(status(uint8_t,group_status,QString)), this, SLOT(status(uint8_t,group_status,QString)));
                    connect(group_os, SIGNAL(progress(uint8_t,uint8_t)), this, SLOT(progress(uint8_t,uint8_t)));

                    mode = ACTION_OS_UPLOAD_RESET;
                    processor->set_transport(active_transport);
                    set_group_transport_settings(group_os);

                    bool started = group_os->start_reset(false);
                    //todo: check status

                    log_debug() << "do reset";
                }
                else
                {
                    log_information() << "Upload finished, set image state failed: command not supported (likely MCUboot serial recovery)";
                }
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
                log_information() << "Echo response: " << error_string;
                error_string = nullptr;
            }
            else if (user_data == ACTION_OS_UPLOAD_RESET)
            {
            }
            else if (user_data == ACTION_OS_RESET)
            {
            }
            else if (user_data == ACTION_OS_MEMORY_POOL)
            {
                uint16_t i = 0;
                uint16_t l = (*this->os_mgmt_memory_pool).length();

                while (i < l)
                {
                    log_information() << i << " - " << (*this->os_mgmt_memory_pool)[i].name;
                    log_information() << "\tSize: " << (*this->os_mgmt_memory_pool)[i].size;
                    log_information() << "\tBlocks: " << (*this->os_mgmt_memory_pool)[i].blocks;
                    log_information() << "\tFree: " << (*this->os_mgmt_memory_pool)[i].free;
                    log_information() << "\tMinimum: " << (*this->os_mgmt_memory_pool)[i].minimum;

                    ++i;
                }
            }
            else if (user_data == ACTION_OS_TASK_STATS)
            {
                uint16_t i = 0;
                uint16_t l = (*this->os_mgmt_task_list).length();

                while (i < l)
                {
                    log_information() << i << "(" << (*this->os_mgmt_task_list)[i].id << ")" << " - " << (*this->os_mgmt_task_list)[i].name;
                    log_information() << "\tContext switches: " << (*this->os_mgmt_task_list)[i].context_switches;
                    log_information() << "\tPriority: " << (*this->os_mgmt_task_list)[i].priority;
                    log_information() << "\tRuntime: " << (*this->os_mgmt_task_list)[i].runtime;
                    log_information() << "\tState: " << (*this->os_mgmt_task_list)[i].state;
                    log_information() << "\tStack usage: " << ((*this->os_mgmt_task_list)[i].stack_usage * sizeof(uint32_t)) << "/" << ((*this->os_mgmt_task_list)[i].stack_size * sizeof(uint32_t));

                    ++i;
                }
            }
            else if (user_data == ACTION_OS_MCUMGR_BUFFER)
            {
                log_information() << "Buffer size: " << os_mgmt_mcumgr_parameters_buffer_size << ", buffer count: " << os_mgmt_mcumgr_parameters_buffer_count;
            }
            else if (user_data == ACTION_OS_OS_APPLICATION_INFO)
            {
                log_information() << os_mgmt_os_application_info_response;
                delete os_mgmt_os_application_info_response;
                os_mgmt_os_application_info_response = nullptr;
            }
            else if (user_data == ACTION_OS_BOOTLOADER_INFO)
            {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
                switch (os_mgmt_bootloader_info_response->typeId())
#else
                switch (os_mgmt_bootloader_info_response->type())
#endif
                {
                    case QMetaType::Bool:
                    {
                        log_information() << (os_mgmt_bootloader_info_response->toBool() == true ? "True" : "False");
                        break;
                    }
                    case QMetaType::Int:
                    {
                        log_information() << os_mgmt_bootloader_info_response->toInt();
                        break;
                    }
                    case QMetaType::LongLong:
                    {
                        log_information() << os_mgmt_bootloader_info_response->toLongLong();
                        break;
                    }
                    case QMetaType::UInt:
                    {
                        log_information() << os_mgmt_bootloader_info_response->toUInt();
                        break;
                    }
                    case QMetaType::ULongLong:
                    {
                        log_information() << os_mgmt_bootloader_info_response->toULongLong();
                        break;
                    }
                    case QMetaType::Double:
                    {
                        log_information() << os_mgmt_bootloader_info_response->toDouble();
                        break;
                    }
                    case QMetaType::QString:
                    {
                        log_information() << os_mgmt_bootloader_info_response->toString();
                        break;
                    }
                    default:
                    {
                        log_information() << "Invalid response type";
                    }
                };
            }
#if 0
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

        if (user_data == ACTION_OS_OS_APPLICATION_INFO)
        {
            delete os_mgmt_os_application_info_response;
            os_mgmt_os_application_info_response = nullptr;
        }
        else if (user_data == ACTION_OS_BOOTLOADER_INFO)
        {
            delete os_mgmt_bootloader_info_response;
            os_mgmt_bootloader_info_response = nullptr;
        }
        else if (user_data == ACTION_OS_TASK_STATS)
        {
            delete os_mgmt_task_list;
            os_mgmt_task_list = nullptr;
        }
        else if (user_data == ACTION_OS_MEMORY_POOL)
        {
            delete os_mgmt_memory_pool;
            os_mgmt_memory_pool = nullptr;
        }
    }
    else if (sender() == group_shell)
    {
        log_debug() << "shell sender";

        if (status == STATUS_COMPLETE)
        {
            log_debug() << "complete";

            if (user_data == ACTION_SHELL_EXECUTE)
            {
                log_information() << error_string.toUtf8();

                if (shell_mgmt_rc == 0)
                {
                    error_string = nullptr;
                }
                else
                {
                    error_string = QString("Finished, error (ret): ").append(QString::number(shell_mgmt_rc));
                }
            }
        }
    }
    else if (sender() == group_stat)
    {
        log_debug() << "stat sender";

        if (status == STATUS_COMPLETE)
        {
            log_debug() << "complete";

            if (user_data == ACTION_STAT_GROUP_DATA)
            {
                uint16_t i = 0;
                uint16_t l = stat_mgmt_stats->length();

                while (i < l)
                {
                    log_information() << (*stat_mgmt_stats)[i].name << ": " << (*stat_mgmt_stats)[i].value;
                    ++i;
                }
            }
            else if (user_data == ACTION_STAT_LIST_GROUPS)
            {
                uint16_t i = 0;
                uint16_t l = stat_mgmt_groups->length();

                while (i < l)
                {
                    log_information() << (*stat_mgmt_groups)[i];
                    ++i;
                }

            }
        }

        if (user_data == ACTION_STAT_GROUP_DATA)
        {
            delete stat_mgmt_stats;
            stat_mgmt_stats = nullptr;
        }
        else if (user_data == ACTION_STAT_LIST_GROUPS)
        {
            delete stat_mgmt_groups;
            stat_mgmt_groups = nullptr;
        }
    }
    else if (sender() == group_fs)
    {
        log_debug() << "fs sender";

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
                log_information() << "Hash/checksum: " << fs_mgmt_hash_checksum->toHex() << ", file size: " << fs_mgmt_file_size;
            }
            else if (user_data == ACTION_FS_SUPPORTED_HASHES_CHECKSUMS)
            {
                uint8_t i = 0;
                uint8_t l = fs_mgmt_supported_hashes_checksums->length();

                while (i < l)
                {
                    log_information() << (*fs_mgmt_supported_hashes_checksums)[i].name;
                    log_information() << "\t" << (*fs_mgmt_supported_hashes_checksums)[i].format;
                    log_information() << "\t" << (*fs_mgmt_supported_hashes_checksums)[i].size;
                    ++i;
                }
            }
            else if (user_data == ACTION_FS_STATUS)
            {
                log_information() << fs_mgmt_file_size;
            }
            else if (user_data == ACTION_FS_CLOSE_FILE)
            {
            }
        }

        if (user_data == ACTION_FS_HASH_CHECKSUM)
        {
            delete fs_mgmt_hash_checksum;
            fs_mgmt_hash_checksum = nullptr;
        }
        else if (user_data == ACTION_FS_SUPPORTED_HASHES_CHECKSUMS)
        {
            delete fs_mgmt_supported_hashes_checksums;
            fs_mgmt_supported_hashes_checksums = nullptr;
        }
    }
#if 0
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
#endif
    else if (sender() == group_enum)
    {
        log_debug() << "enum sender";

        if (status == STATUS_COMPLETE)
        {
            log_debug() << "complete";

            if (user_data == ACTION_ENUM_COUNT)
            {
                log_information() << enum_mgmt_count;
            }
            else if (user_data == ACTION_ENUM_LIST)
            {
                QString groups;
                uint8_t i = 0;
                uint8_t l = enum_mgmt_group_ids->length();

                while (i < l)
                {
                    groups.append(QString::number(enum_mgmt_group_ids->at(i)));
                    ++i;

                    if (i < l)
                    {
                        groups.append(", ");
                    }
                }

                log_information() << groups;
            }
            else if (user_data == ACTION_ENUM_SINGLE)
            {
                log_information() << enum_mgmt_id << ", " << (enum_mgmt_end == true ? "at end" : "more groups present");
            }
            else if (user_data == ACTION_ENUM_DETAILS)
            {
                uint16_t i = 0;
                uint16_t l = (*enum_mgmt_group_details).length();

                while (i < l)
                {
                    log_information() << "ID " << (*enum_mgmt_group_details)[i].id << "(" << (*enum_mgmt_group_details)[i].name << ")" << " with " << (*enum_mgmt_group_details)[i].handlers << " handlers";
                    ++i;
                }
            }
        }

        if (user_data == ACTION_ENUM_LIST)
        {
            delete enum_mgmt_group_ids;
            enum_mgmt_group_ids = nullptr;
        }
        else if (user_data == ACTION_ENUM_DETAILS)
        {
            delete enum_mgmt_group_details;
            enum_mgmt_group_details = nullptr;
        }
    }

    if (finished == true)
    {
        mode = ACTION_IDLE;

        if (error_string == nullptr)
        {
            if (status == STATUS_COMPLETE)
            {
                error_string = tr("Finished");
            }
            else if (status == STATUS_ERROR)
            {
                error_string = tr("Error");
            }
            else if (status == STATUS_TIMEOUT)
            {
                error_string = tr("Command timed out");
            }
            else if (status == STATUS_CANCELLED)
            {
                error_string = tr("Cancelled");
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

void command_processor::interactive_thread_started()
{
    qDebug() << "Interactive mode ready!";

    text_thread_wait_condition.wakeAll();
}

void command_processor::interactive_thread_data(QString data)
{
    run(QStringList() << "" << data.split(" ", Qt::SkipEmptyParts));
}

void command_processor::interactive_mode()
{
    if (is_interactive_mode == false)
    {
        QEventLoop wait_loop;

        is_interactive_mode = true;

        connect(&text_thread_object, SIGNAL(data(QString)), this, SLOT(interactive_thread_data(QString)), Qt::QueuedConnection);
        connect(&text_thread_object, SIGNAL(started()), this, SLOT(interactive_thread_started()));

        text_thread_object.start();
        wait_loop.exec();
    }
}

void command_processor::return_status(int status)
{
    if (is_interactive_mode == false)
    {
        return QCoreApplication::exit(status);
    }

    qDebug() << "Status: " << status;
    text_thread_wait_condition.wakeAll();
}

/******************************************************************************/
// END OF FILE
/******************************************************************************/
