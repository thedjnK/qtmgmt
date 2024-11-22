/******************************************************************************
** Copyright (C) 2021-2024 Jamie M.
**
** Project: qtmgmt
**
** Module:  smp_uart.h
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
#ifndef SMP_UART_H
#define SMP_UART_H

/******************************************************************************/
// Include Files
/******************************************************************************/
#include <QObject>
#include <QSerialPort>
#include <smp_transport.h>
#include <smp_message.h>
#include <debug_logger.h>

/******************************************************************************/
// Enum typedefs
/******************************************************************************/
enum smp_uart_flow_control_t {
    SMP_UART_FLOW_CONTROL_NONE,
    SMP_UART_FLOW_CONTROL_SOFTWARE,
    SMP_UART_FLOW_CONTROL_HARDWARE,

    SMP_UART_FLOW_CONTROL_COUNT
};

enum smp_uart_parity_t {
    SMP_UART_PARITY_NONE,
    SMP_UART_PARITY_EVEN,
    SMP_UART_PARITY_ODD,
    SMP_UART_PARITY_SPACE,
    SMP_UART_PARITY_MARK,

    SMP_UART_PARITY_COUNT
};

enum smp_uart_data_bits_t {
    SMP_UART_DATA_BITS_7,
    SMP_UART_DATA_BITS_8,

    SMP_UART_DATA_BITS_COUNT
};

enum smp_uart_stop_bits_t {
    SMP_UART_STOP_BITS_1,
    SMP_UART_STOP_BITS_1_AND_HALF,
    SMP_UART_STOP_BITS_2,

    SMP_UART_STOP_BITS_COUNT
};

/******************************************************************************/
// Forward declaration of Class, Struct & Unions
/******************************************************************************/
struct smp_uart_config_t {
    QString port_name;
    uint32_t baud;
    enum smp_uart_flow_control_t flow_control;
    enum smp_uart_parity_t parity;
    enum smp_uart_data_bits_t data_bits;
    enum smp_uart_stop_bits_t stop_bits;
};

/******************************************************************************/
// Class definitions
/******************************************************************************/
class smp_uart : public smp_transport
{
    Q_OBJECT

public:
    smp_uart(QObject *parent = nullptr);
    ~smp_uart();
    int connect(void) override;
    int disconnect(bool force) override;
    int is_connected() override;
    int set_connection_config(struct smp_uart_config_t *configuration);
    smp_transport_error_t send(smp_message *message) override;
    uint16_t max_message_data_size(uint16_t mtu) override;

private:
    void data_received(QByteArray *message);

signals:
    void serial_write(QByteArray *data);

private slots:
    void serial_read();
    void serial_error(QSerialPort::SerialPortError error);

private:
    struct smp_uart_config_t serial_config;
    bool serial_config_set;
    QSerialPort serial_port;
    QByteArray SerialData;
    QByteArray SMPBuffer;
    QByteArray SMPBufferActualData;
    bool SMPWaitingForContinuation = false;
    const QByteArray smp_first_header = QByteArrayLiteral("\x06\x09");
    const QByteArray smp_continuation_header = QByteArrayLiteral("\x04\x14");
    uint16_t waiting_packet_length = 0;
};

#endif // SMP_UART_H

/******************************************************************************/
// END OF FILE
/******************************************************************************/
