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

#include "midi.h"
#include "pico/stdlib.h"
#include "bsp/board.h"
#include "bsp/board_api.h"

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
    uint8_t buffer[128];
    uint8_t buffer_size = 0;

    std::function<void()> onClockCallback, onClockStartCallback, onClockStopCallback;

    void registerMIDIByte(uint8_t byte);
    void clearBuffer();

public:
    MIDIInterface(uint8_t pinTX, uint8_t pinRX);
    void init();
    void sendMIDINBytesUART(uint8_t *packet, int n);
    void sendMIDIStatusUART(uint8_t status, uint8_t note, uint8_t velocity);
    void sendMIDIStatusUSB(uint8_t status, uint8_t note, uint8_t velocity);
    void sendNoteOnUART(uint8_t channel, uint8_t note, uint8_t velocity);
    void sendCCUART(uint8_t channel, uint8_t control, uint8_t value);
    void sendNoteOnUSB(uint8_t channel, uint8_t note, uint8_t velocity);
    void sendCCUSB(uint8_t channel, uint8_t control, uint8_t value);
    bool update();
    int midiAvailableUSB();
    bool midiAvailableUART();
    bool midiAvailable();
    bool getMIDIUSB(uint8_t *packet);
    void getMIDIUART(uint8_t *packet);
    bool getMIDI(uint8_t *packet);
    void initUART(PIO pio_tx_, PIO pio_rx_, uint sm_tx, uint sm_rx, uint pin_tx_, uint pin_rx_);

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