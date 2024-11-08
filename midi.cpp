#include "midi.h"

MIDIInterface::MIDIInterface(uint8_t pinTX, uint8_t pinRX)
{
    PIN_TX = pinTX;
    PIN_RX = pinRX;
}

void MIDIInterface::init()
{

    // // init device stack on configured roothub port
    // tusb_rhport_init_t dev_init = {
    //     .role = TUSB_ROLE_DEVICE,
    //     .speed = TUSB_SPEED_AUTO};
    // tusb_init(BOARD_TUD_RHPORT, &dev_init);

    if (board_init_after_tusb)
    {
        board_init_after_tusb();
    }
}

void MIDIInterface::sendMIDINBytesUART(uint8_t *packet, int n)
{
    for (int i = 0; i < n; i++)
    {
        uart_tx_program_putc(pio_tx, sm_tx, packet[i]);
    }
}

void MIDIInterface::sendMIDIStatusUART(uint8_t status, uint8_t data1, uint8_t data2)
{
    uart_tx_program_putc(pio_tx, sm_tx, status);
    uart_tx_program_putc(pio_tx, sm_tx, data1);
    uart_tx_program_putc(pio_tx, sm_tx, data2);
}

void MIDIInterface::sendNoteOnUART(uint8_t channel, uint8_t note, uint8_t velocity)
{
    uint8_t note_on[3] = {0x90 | channel, note, velocity};
    sendMIDIStatusUART(note_on[0], note_on[1], note_on[2]);
}

void MIDIInterface::sendCCUART(uint8_t channel, uint8_t control, uint8_t value)
{
    uint8_t cc[3] = {0xB0 | channel, control, value};
    sendMIDIStatusUART(cc[0], cc[1], cc[2]);
}

void MIDIInterface::sendMIDIStatusUSB(uint8_t status, uint8_t data1, uint8_t data2)
{
    uint8_t pack[3] = {status, data1, data2};
    tud_midi_stream_write(CABLE_NUMBER, pack, 3);
}

void MIDIInterface::sendNoteOnUSB(uint8_t channel, uint8_t note, uint8_t velocity)
{
    sendMIDIStatusUSB(0x90 | channel, note, velocity);
}

void MIDIInterface::sendCCUSB(uint8_t channel, uint8_t control, uint8_t value)
{
    sendMIDIStatusUSB(0xB0 | channel, control, value);
}

int MIDIInterface::midiAvailableUSB()
{
    return tud_midi_available();
}

bool MIDIInterface::update()
{
    if (buffer_size == 0)
        return false;

    uint8_t status = buffer[0] & 0xF0;
    switch (status)
    {
    case 0x80:
    case 0x90:
    case 0xB0:
    {
        return buffer_size == 3;
    }
    case 0xC0:
    {
        return buffer_size == 2;
    }
    case 0xF0:
    {
        switch (buffer[0])
        {
        case 0xF0:
        {
            for (uint8_t i = 0; i < 127; i++)
            {
                if (buffer[i] == 0xF7)
                {
                    return true;
                }
            }
            return false;
            break;
        }
        case 0xF1:
        case 0xF3:
        {
            return buffer_size == 2;
            break;
        }
        case 0xF2:
        {
            return buffer_size == 3;
            break;
        }
        case 0xF7:
        {
            clearBuffer();
            break;
        }
        case 0xF8:
        {
            if (onClockCallback)
            {
                onClockCallback();
            }
            return true;
        }
        case 0xFA:
        {
            if (onClockStartCallback)
            {
                onClockStartCallback();
            }
            return true;
        }
        case 0xFC:
        {
            if (onClockStopCallback)
            {
                onClockStopCallback();
            }
            return true;
        }

        default:
            return true;
        }
    }
    }
}

void MIDIInterface::clearBuffer()
{
    buffer_size = 0;
}

bool MIDIInterface::midiAvailableUART()
{
    return pio_sm_get_rx_fifo_level(pio_rx, sm_rx) > 0;
}

bool MIDIInterface::midiAvailable()
{
    return midiAvailableUSB() || midiAvailableUART();
}

bool MIDIInterface::getMIDIUSB(uint8_t *packet)
{
    if (tud_midi_available())
    {
        tud_midi_packet_read(packet);
        // tud_midi_stream_read(packet, 3);
        return true;
    }
    // tud_midi_packet_read(packet);
    // tud_midi_stream_read(packet);
    // uint8_t tmp[4];
    // if (midiAvailableUSB())
    // {
    //     tud_midi_stream_read(tmp);
    //     packet[0] = tmp[1];
    //     packet[1] = tmp[2];
    //     packet[2] = tmp[3];

    //     return true;
    // }
    // return false;
}

void MIDIInterface::getMIDIUART(uint8_t *packet)
{

    if (pio_sm_get_rx_fifo_level(pio_rx, sm_rx) > 0)
    {

        uint8_t header = uart_rx_program_getc(pio_rx, sm_rx);
        uint8_t status = header & 0xF0;
        switch (status)
        {
        case 0x90:
        case 0x80:
        case 0xB0:
        {
            packet[0] = header;
            packet[1] = uart_rx_program_getc(pio_rx, sm_rx);
            packet[2] = uart_rx_program_getc(pio_rx, sm_rx);
            break;
        }
        case 0xC0:
        {
            packet[0] = header;
            packet[1] = uart_rx_program_getc(pio_rx, sm_rx);
            break;
        }
        case 0xF0:
        {
            packet[0] = header;
            break;
        }
        default:
            break;
        }
    }
}

bool MIDIInterface::getMIDI(uint8_t *packet)
{
    if (midiAvailableUSB())
    {
        getMIDIUSB(packet);
        return true;
    }
    if (midiAvailableUART())
    {
        getMIDIUART(packet);
        return true;
    }
    return false;
}

void MIDIInterface::initUART(PIO pio_tx_, PIO pio_rx_, uint sm_tx_, uint sm_rx_, uint pin_tx_, uint pin_rx_)

{
    pio_tx = pio_tx_;
    pio_rx = pio_rx_;
    sm_tx = sm_tx_;
    sm_rx = sm_rx_;
    pin_tx = pin_tx_;
    pin_rx = pin_rx_;

    uint offset2 = pio_add_program(pio_tx, &uart_tx_program);
    uart_tx_program_init(pio_tx, sm_tx, offset2, pin_tx, MIDI_SERIAL_BAUD);
    pio_sm_set_enabled(pio_tx, sm_tx, true);

    offset2 = pio_add_program(pio_rx, &uart_rx_program);
    uart_rx_program_init(pio_rx, sm_rx, offset2, pin_rx, MIDI_SERIAL_BAUD);
    pio_sm_set_enabled(pio_rx, sm_rx, true);
}

void MIDIInterface::registerMIDIByte(uint8_t byte)
{
    buffer[++buffer_size] = byte;
    if (buffer_size > 127)
    {
        buffer_size = 0;
    }
}

void MIDIInterface::onClock(std::function<void()> callback)
{
    onClockCallback = callback;
}

void MIDIInterface::onClockStart(std::function<void()> callback)
{
    onClockStartCallback = callback;
}

void MIDIInterface::onClockStop(std::function<void()> callback)
{
    onClockStopCallback = callback;
}

void tud_mount_cb(void)
{
}

void tud_umount_cb(void)
{
}

void tud_suspend_cb(bool remote_wakeup_en)
{
}

void tud_resume_cb(void)
{
}
