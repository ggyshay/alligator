#include <functional>

#ifndef MIDIINTERFACE_H
#define MIDIINTERFACE_H
#include "uart_rx.pio.h"
#include "uart_tx.pio.h"

#define CABLE_NUMBER 0
#define MIDI_SERIAL_BAUD 31250

#define CLOCK_START 0xFA
#define CLOCK_STOP 0xFC
#define CLOCK_SIGNAL 0xF8

#define RX_NON_EMPTY_INTERUPT_SOURCE pis_sm1_rx_fifo_not_empty

#include "midi.h"
#include "pico/stdlib.h"
#include "bsp/board.h"
#include "bsp/board_api.h"
#include "circular_buffer.h"

#include "tusb.h"

class MIDIInterface
{
private:
    uint8_t PIN_TX;
    uint8_t PIN_RX;
    PIO pio_tx;
    PIO pio_rx;
    uint sm_tx;
    uint sm_rx;
    uint pin_tx;
    uint pin_rx;

    // uint8_t buffer[128];
    // uint8_t write_ptr = 0;
    // uint8_t read_ptr = 0;

    // uint8_t uart_rx_buffer[128];
    // uint8_t uart_write_ptr = 0;
    // uint8_t uart_read_ptr = 0;

    // uint8_t buffer_clean[128];
    // uint8_t clean_write_ptr = 0;
    // uint8_t clean_read_ptr = 0;

    std::function<void()> onClockCallback, onClockStartCallback, onClockStopCallback;

    static MIDIInterface *instance;
    // void clearBuffer();
    // void writeBuffer(uint8_t *packet, int n);
    static void static_register_uart_rx();
    void register_uart_rx();
    void register_clean(uint8_t start, uint8_t len);
    bool pull();

public:
    MIDIInterface(uint8_t pinTX, uint8_t pinRX);
    CircularBuffer usb_buffer, uart_buffer, clean_buffer;
    void init();
    void initUART(PIO pio_tx_, PIO pio_rx_, uint sm_tx, uint sm_rx, uint pin_tx_, uint pin_rx_);

    int midiAvailableUART();
    int midiAvailableUSB();

    bool update();

    void getMIDIUART(uint8_t *packet);
    bool getMIDIUSB(uint8_t *packet);

    void sendMIDINBytesUART(uint8_t *packet, int n);
    void sendMIDINBytesUSB(uint8_t *packet, int n);

    void onClock(std::function<void()>);
    void onClockStart(std::function<void()>);
    void onClockStop(std::function<void()>);
};

// ====================================
// Tiny USB MIDI Callbacks
// ====================================

void tud_mount_cb(void);
void tud_umount_cb(void);
void tud_suspend_cb(bool remote_wakeup_en);
void tud_resume_cb(void);

#endif