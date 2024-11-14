#include "midi.h"
MIDIInterface *MIDIInterface::instance = nullptr;

MIDIInterface::MIDIInterface(uint8_t pinTX, uint8_t pinRX)
{
    PIN_TX = pinTX;
    PIN_RX = pinRX;
}

void MIDIInterface::init()
{
    if (board_init_after_tusb)
    {
        board_init_after_tusb();
    }
    instance = this;
}

void MIDIInterface::initUART(PIO pio_tx_, PIO pio_rx_, uint sm_tx_, uint sm_rx_, uint pin_tx_, uint pin_rx_)

{
    instance = this;
    pio_tx = pio_tx_;
    pio_rx = pio_rx_;
    sm_tx = sm_tx_;
    sm_rx = sm_rx_;
    pin_tx = pin_tx_;
    pin_rx = pin_rx_;

    uint offset2 = pio_add_program(pio_tx, &uart_tx_program);
    uart_tx_program_init(pio_tx, sm_tx, offset2, pin_tx, MIDI_SERIAL_BAUD);
    pio_sm_set_enabled(pio_tx, sm_tx, true);
    pio_sm_clear_fifos(pio_tx, sm_tx);
    pio_sm_restart(pio_tx, sm_tx);

    offset2 = pio_add_program(pio_rx, &uart_rx_program);
    uart_rx_program_init(pio_rx, sm_rx, offset2, pin_rx, MIDI_SERIAL_BAUD);
    pio_sm_set_enabled(pio_rx, sm_rx, true);
    pio_sm_clear_fifos(pio_rx, sm_rx);
    pio_sm_restart(pio_rx, sm_rx);

    irq_set_exclusive_handler(PIO1_IRQ_0, static_register_uart_rx);
    pio_set_irq0_source_enabled(pio_rx, RX_NON_EMPTY_INTERUPT_SOURCE, true);
    irq_set_enabled(PIO1_IRQ_0, true);
    irq_set_priority(PIO1_IRQ_0, 0);
}

int MIDIInterface::midiAvailableUART()
{
    return uart_buffer.available();
}

int MIDIInterface::midiAvailableUSB()
{
    return clean_buffer.available();
}

bool MIDIInterface::pull()
{
    int n_available = tud_midi_available();
    if (n_available > 0)
    {

        uint8_t packet[64] = {0};
        tud_midi_stream_read(packet, 64);

        usb_buffer.write(packet, n_available);
        return true;
    }
    return false;
}

bool MIDIInterface::update()
{
    pull();

    while (usb_buffer.available())
    {
        uint8_t buffer_size = usb_buffer.available();
        if (buffer_size == 0)
        {
            return false;
        }

        uint8_t value = usb_buffer.peak();
        uint8_t status = value & 0xF0;
        switch (status)
        {
        case 0x80:
        case 0x90:
        case 0xB0:
        {
            if (buffer_size < 3)
            {
                return true;
            }
            if ((status == 0x90) && onNoteOnCallback)
            {
                onNoteOnCallback(status, usb_buffer.peak(1), usb_buffer.peak(2));
            }

            clean_buffer.get_from_buffer(&usb_buffer, 3);

            break;
        }
        case 0xC0:
        {
            if (buffer_size < 2)
            {
                return true;
            }

            clean_buffer.get_from_buffer(&usb_buffer, 2);

            break;
        }
        case 0xF0:
        {
            switch (value)
            {
            case 0xF0:
            {
                uint8_t i = 1;
                while (usb_buffer.peak(i) != 0xF7)
                {
                    i++;
                    if (i == 128)
                    {
                        return true;
                    }
                };

                clean_buffer.get_from_buffer(&usb_buffer, i);

                break;
            }
            case 0xF1:
            case 0xF3:
            {
                if (buffer_size < 2)
                {
                    return true;
                }

                clean_buffer.get_from_buffer(&usb_buffer, 2);

                break;
            }
            case 0xF2:
            {
                if (buffer_size < 3)
                {
                    return true;
                }
                clean_buffer.get_from_buffer(&usb_buffer, 3);

                break;
            }
            case 0xF7:
            {

                usb_buffer.clear();

                break;
            }
            case 0xF8:
            {
                if (onClockCallback)
                {
                    onClockCallback();
                }

                clean_buffer.get_from_buffer(&usb_buffer, 1);

                break;
            }
            case 0xFA:
            {
                if (onClockStartCallback)
                {
                    onClockStartCallback();
                }

                clean_buffer.get_from_buffer(&usb_buffer, 1);

                break;
            }
            case 0xFC:
            {
                if (onClockStopCallback)
                {
                    onClockStopCallback();
                }

                clean_buffer.get_from_buffer(&usb_buffer, 1);

                break;
            }

            default:
            {
                uint8_t k = usb_buffer.read();

                break;
            }
            }
            break;
        }
        case 0x00:
        {
            uint8_t k = usb_buffer.read();

            break;
        }
        default:
        {
            uint8_t k = usb_buffer.read();

            break;
        }
        }
    }
    return false;
}

void MIDIInterface::getMIDIUART(uint8_t *packet)
{
    int n_available = midiAvailableUART();
    uart_buffer.read(packet, n_available);
}

bool MIDIInterface::getMIDIUSB(uint8_t *packet)
{
    int n_available = clean_buffer.available();
    clean_buffer.read(packet, n_available);

    return false;
}

void MIDIInterface::sendMIDINBytesUART(uint8_t *packet, int n)
{
    for (int i = 0; i < n; i++)
    {
        uart_tx_program_putc(pio_tx, sm_tx, packet[i]);
    }
}

void MIDIInterface::sendMIDINBytesUSB(uint8_t *packet, int n)
{
    tud_midi_stream_write(CABLE_NUMBER, packet, n);
}

void MIDIInterface::static_register_uart_rx()
{
    instance->register_uart_rx();
}

void MIDIInterface::register_uart_rx()
{
    while (pio_sm_get_rx_fifo_level(pio_rx, sm_rx) > 0)
    {
        uint8_t r = uart_rx_program_getc(pio_rx, sm_rx);
        uart_buffer.write(r);
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

void MIDIInterface::onNoteOn(std::function<void(uint8_t, uint8_t, uint8_t)> callback)
{
    onNoteOnCallback = callback;
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
