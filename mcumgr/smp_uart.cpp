/******************************************************************************
** Copyright (C) 2021-2024 Jamie M.
**
** Project: qtmgmt
**
** Module:  smp_uart.cpp
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
#include "smp_uart.h"
#include <crc16.h>
#include <math.h>

smp_uart::smp_uart(QObject *parent)
{
    Q_UNUSED(parent);

    //connect(&serial_port, SIGNAL(bytesWritten(qint64)));
    //connect(&serial_port, SIGNAL(aboutToClose());
    QObject::connect(&serial_port, SIGNAL(readyRead()), this, SLOT(serial_read()));
    QObject::connect(&serial_port, SIGNAL(errorOccurred(QSerialPort::SerialPortError)), this, SLOT(serial_error(QSerialPort::SerialPortError)));

    serial_config_set = false;
}

smp_uart::~smp_uart()
{
    QObject::disconnect(&serial_port, SIGNAL(readyRead()), this, SLOT(serial_read()));
    QObject::disconnect(&serial_port, SIGNAL(errorOccurred(QSerialPort::SerialPortError)), this, SLOT(serial_error(QSerialPort::SerialPortError)));

    if (serial_port.isOpen())
    {
        serial_port.close();
    }
}

int smp_uart::connect(void)
{
    if (serial_port.isOpen() == true)
    {
        return SMP_TRANSPORT_ERROR_ALREADY_CONNECTED;
    }

    if (serial_config_set == false || serial_config.port_name.length() == 0 || serial_config.flow_control > SMP_UART_FLOW_CONTROL_COUNT || serial_config.parity > SMP_UART_PARITY_COUNT || serial_config.data_bits > SMP_UART_DATA_BITS_COUNT || serial_config.stop_bits > SMP_UART_STOP_BITS_COUNT)
    {
        return SMP_TRANSPORT_ERROR_INVALID_CONFIGURATION;
    }

    //Port selected: setup serial port
    serial_port.setPortName(serial_config.port_name);
    serial_port.setBaudRate(serial_config.baud);
    serial_port.setDataBits(serial_config.data_bits == SMP_UART_DATA_BITS_8 ? QSerialPort::Data8 : QSerialPort::Data7);
    serial_port.setStopBits((serial_config.stop_bits == SMP_UART_STOP_BITS_1 ? QSerialPort::OneStop : (serial_config.stop_bits == SMP_UART_STOP_BITS_1_AND_HALF ? QSerialPort::OneAndHalfStop : QSerialPort::TwoStop)));
    serial_port.setParity((serial_config.parity == SMP_UART_PARITY_NONE ? QSerialPort::NoParity : (serial_config.parity == SMP_UART_PARITY_EVEN ? QSerialPort::EvenParity : (serial_config.parity == SMP_UART_PARITY_ODD ? QSerialPort::OddParity : (serial_config.parity == SMP_UART_PARITY_SPACE ? QSerialPort::SpaceParity : QSerialPort::MarkParity)))));
    serial_port.setFlowControl((serial_config.flow_control == SMP_UART_FLOW_CONTROL_NONE ? QSerialPort::NoFlowControl : (serial_config.flow_control == SMP_UART_FLOW_CONTROL_HARDWARE ? QSerialPort::HardwareControl : QSerialPort::SoftwareControl)));

    if (serial_port.open(QIODevice::ReadWrite))
    {
        //Successful, set RTS to be asserted if hardware flow control is not used
        if (serial_config.flow_control != SMP_UART_FLOW_CONTROL_HARDWARE)
        {
            serial_port.setRequestToSend(true);
        }

        return SMP_TRANSPORT_ERROR_OK;
    }
    else
    {
        //Error whilst opening
        //ui->statusBar->showMessage("Error: ");
        //ui->statusBar->showMessage(ui->statusBar->currentMessage().append(serial_port.errorString()));
    }

    return SMP_TRANSPORT_ERROR_OPEN_FAILED;
}

int smp_uart::disconnect(bool force)
{
    Q_UNUSED(force);

    if (serial_port.isOpen() == false)
    {
        return SMP_TRANSPORT_ERROR_NOT_CONNECTED;
    }

    SerialData.clear();
    SMPBuffer.clear();
    SMPBufferActualData.clear();
    SMPWaitingForContinuation = false;
    waiting_packet_length = 0;
    serial_port.close();

    return SMP_TRANSPORT_ERROR_OK;
}

int smp_uart::is_connected()
{
    if (serial_port.isOpen() == true)
    {
        return 1;
    }

    return 0;
}

void smp_uart::data_received(QByteArray *message)
{
    smp_message full_message;
    full_message.append(message);

    if (full_message.is_valid())
    {
        emit receive_waiting(&full_message);
        full_message.clear();
    }
}

void smp_uart::serial_read()
{
    SerialData.append(serial_port.readAll());

    //Search for SMP packets
    int32_t pos = SerialData.indexOf(smp_first_header);
    int32_t pos_other = SerialData.indexOf(smp_continuation_header);
    int32_t posA = SerialData.indexOf(0x0a, pos + 2);
    int32_t posA_other = SerialData.indexOf(0x0a, pos_other + 2);

    while ((pos != -1 && posA != -1) || (pos_other != -1 && posA_other != -1))
//        while (pos != -1 && posA != -1 /*&& SMPWaitingForContinuation == false*/)
    {
        if (pos >= 0 && (pos_other == -1 || pos < pos_other))
        {
            //Start
            //Check this header
            SMPBuffer.clear();
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
            SMPBuffer = QByteArray::fromBase64(SerialData.mid((pos + 2), (posA - pos - 2)), QByteArray::AbortOnBase64DecodingErrors);
#else
            SMPBuffer = QByteArray::fromBase64(SerialData.mid((pos + 2), (posA - pos - 2)));
#endif

            if (SMPBuffer.length() == 0)
            {
                log_error() << "Failed decoding base64";
            }
            else if (SMPBuffer.length() > 2)
            {
                //Check length
                waiting_packet_length = ((uint16_t)SMPBuffer[0]) << 8;
                waiting_packet_length |= ((uint16_t)SMPBuffer[1] & 0xff);
                SMPBuffer.remove(0, 2);
                if (SMPBuffer.length() >= (waiting_packet_length))
                {
                    //We have a full packet, check the checksum
                    uint16_t crc = crc16(&SMPBuffer, 0, SMPBuffer.length() - 2, 0x1021, 0, true);
                    uint16_t message_crc = ((uint16_t)SMPBuffer[(SMPBuffer.length() - 2)]) << 8;
                    message_crc |= SMPBuffer[(SMPBuffer.length() - 1)] & 0xff;

                    if (crc == message_crc)
                    {
                        //Good to parse message after removing CRC
                        SMPBuffer.remove((SMPBuffer.length() - 2), 2);
                        data_received(&SMPBuffer);
                    }
                    else
                    {
                        //CRC failure
                        log_error() << "CRC failure, expected " << message_crc << " but got " << crc;
                    }
                }
                else
                {
                    //More data expected in another packet
                    SMPWaitingForContinuation = true;
                    SMPBufferActualData = SMPBuffer;
                }
            }

            SerialData.remove(pos, (posA - pos + 1));

            pos = SerialData.indexOf(smp_first_header);
            posA = SerialData.indexOf(0x0a, pos + 2);
            pos_other = SerialData.indexOf(smp_continuation_header);
            posA_other = SerialData.indexOf(0x0a, pos_other + 2);
        }
        else if (SMPWaitingForContinuation == true)
        {
            //Continuation
            //Check this header
            SMPBuffer.clear();
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
            SMPBuffer = QByteArray::fromBase64(SerialData.mid((pos_other + 2), (posA_other - pos_other - 2)), QByteArray::AbortOnBase64DecodingErrors);
#else
            SMPBuffer = QByteArray::fromBase64(SerialData.mid((pos_other + 2), (posA_other - pos_other - 2)));
#endif

            if (SMPBuffer.length() == 0)
            {
                log_error() << "Failed decoding base64";
            }
            else if (SMPBuffer.length() > 0)
            {
                //Check length
                SMPBufferActualData.append(SMPBuffer);
                if (SMPBufferActualData.length() >= (waiting_packet_length /*+ 2*/))
                {
                    //We have a full packet, check the checksum
                    uint16_t crc = crc16(&SMPBufferActualData, 0, SMPBufferActualData.length() - 2, 0x1021, 0, true);
                    uint16_t message_crc = ((uint16_t)SMPBufferActualData[(SMPBufferActualData.length() - 2)]) << 8;
                    message_crc |= SMPBufferActualData[(SMPBufferActualData.length() - 1)] & 0xff;

                    if (crc == message_crc)
                    {
                        //Good to parse message after removing CRC
                        SMPBufferActualData.remove((SMPBufferActualData.length() - 2), 2);
                        data_received(&SMPBufferActualData);
                    }
                    else
                    {
                        //CRC failure
                        log_error() << "CRC failure, expected " << message_crc << " but got " << crc;
                    }

                    SMPBufferActualData.clear();
                    SMPWaitingForContinuation = false;
                }
                else
                {
                    //More data expected in another packet
                    SMPWaitingForContinuation = true;
                }
            }

            SerialData.remove(pos_other, (posA_other - pos_other + 1));

            pos = SerialData.indexOf(smp_first_header);
            if (pos == -1)
            {
                posA = -1;
            }
            else
            {
                posA = SerialData.indexOf(0x0a, pos + 2);
            }
            pos_other = SerialData.indexOf(smp_continuation_header);
            if (pos_other == -1)
            {
                posA_other = -1;
            }
            else
            {
                posA_other = SerialData.indexOf(0x0a, pos_other + 2);
            }
        }
    }

    if (SerialData.length() > 10 && SerialData.indexOf(smp_first_header) == -1 && SerialData.indexOf(smp_continuation_header) == -1)
    {
        log_error() << "Cleared garbage data in UART SMP transport buffer";
        SerialData.clear();
    }
}

smp_transport_error_t smp_uart::send(smp_message *message)
{
    //127 bytes = 3 + base 64 message
    //base64 = 4 bytes output per 3 byte input
    QByteArray output;
    uint16_t size = message->size();
    size += 2;
    output.append((uint8_t)((size & 0xff00) >> 8));
    output.append((uint8_t)(size & 0xff));
    uint16_t crc = crc16(message->data(), 0, message->size(), 0x1021, 0, true);

    QByteArray inbase;
    inbase.append(smp_first_header);
    int32_t pos = 0;

    while (pos < (message->size() + 1))
    {
        /* Chunking required */
        int16_t chunk_size = 93 - output.length();

        if ((chunk_size + pos) > message->size())
        {
            chunk_size = message->size() - pos;

            if (chunk_size == 0)
            {
                goto end;
            }
        }

        output.append(message->data()->mid(pos, chunk_size));
        pos += chunk_size;

        if (pos == message->size() && (93 - chunk_size) > 2)
        {
end:
            output.append((uint8_t)((crc & 0xff00) >> 8));
            output.append((uint8_t)(crc & 0xff));
            pos += 2;
        }

        inbase.append(output.toBase64());
        inbase.append((uint8_t)0x0a);
        serial_port.write(inbase);

        inbase.clear();
        inbase.append(smp_continuation_header);
        output.clear();
    }

    return SMP_TRANSPORT_ERROR_OK;
}

uint16_t smp_uart::max_message_data_size(uint16_t mtu)
{
    float available_mtu = mtu;
    int packets = ceil(available_mtu / 124.0);

    //Convert to number of base64 encoded bytes
    available_mtu = available_mtu * 3.0 / 4.0;

    //Remove packet length and CRC (2 bytes each)
    available_mtu -= 4.0;

    //Remove header and footer of each packet
    available_mtu -= (float)packets * 3.0;

    //Remove possible padding bytes for narrow final packets
    if (((uint16_t)available_mtu % 93) >= 91)
    {
        available_mtu -= 3.0;
    }
    else if (((uint16_t)available_mtu % 93) >= 88)
    {
        available_mtu -= 1.0;
    }

    return (uint16_t)available_mtu;
}

void smp_uart::serial_error(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::NoError)
    {
        return;
    }

    log_error() << "Serial port error: " << error;
    emit smp_transport::error(error);

    if (serial_port.isOpen())
    {
        serial_port.close();
    }
}

int smp_uart::set_connection_config(struct smp_uart_config_t *configuration)
{
    if (serial_port.isOpen() == true)
    {
        return SMP_TRANSPORT_ERROR_ALREADY_CONNECTED;
    }

    serial_config.port_name = configuration->port_name;
    serial_config.baud = configuration->baud;
    serial_config.flow_control = configuration->flow_control;
    serial_config.parity = configuration->parity;
    serial_config.data_bits = configuration->data_bits;
    serial_config.stop_bits = configuration->stop_bits;
    serial_config_set = true;

    return SMP_TRANSPORT_ERROR_OK;
}

QString smp_uart::to_error_string(int error_code)
{
    switch (error_code)
    {
        case QSerialPort::NoError:
        {
            return "No error";
        }
        case QSerialPort::DeviceNotFoundError:
        {
            return "Device no found";
        }
        case QSerialPort::PermissionError:
        {
            return "Permission error";
        }
        case QSerialPort::OpenError:
        {
            return "Open error";
        }
        case QSerialPort::WriteError:
        {
            return "Write error";
        }
        case QSerialPort::ReadError:
        {
            return "Read error";
        }
        case QSerialPort::ResourceError:
        {
            return "Resource error";
        }
        case QSerialPort::UnsupportedOperationError:
        {
            return "Unsupported operation error";
        }
        case QSerialPort::UnknownError:
        {
            return "Unknown error";
        }
        case QSerialPort::TimeoutError:
        {
            return "Timeout error";
        }
        case QSerialPort::NotOpenError:
        {
            return "Device not open";
        }
        default:
        {
            return "Unhandled error code";
        }
    };
}
