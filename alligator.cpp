#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "src/led_matrix.h"
#include "src/midi.h"

#define millis() to_ms_since_boot(get_absolute_time())
#define CABLE_NUM 0

#define TX_PIO pio0
#define RX_PIO pio1
#define LEDS_PIO pio1

#define TX_SM 0
#define RX_SM 1
#define LEDS_SM 2

const uint WS2812_PIN = 2;
const uint PIN_TX = 3;
const uint PIN_RX = 4;

uint32_t lastSent = 0;
uint32_t lastClock = 0;
int pulseCounter = 0;
const int sequence_length = 16 * 24;
bool MIDIIsLocked = false;

MIDIInterface midiInt(PIN_TX, PIN_RX);
uint8_t packet[128];
int t = 0;

LedMatrix ledMatrix;

void clearPacket()
{
    for (int i = 0; i < 128; i++)
    {
        packet[i] = 0;
    }
}

void midi_task()
{
    midiInt.update();
    clearPacket();
    int n_available = midiInt.midiAvailableUSB();
    if (n_available > 0)
    {

        midiInt.getMIDIUSB(packet);

        midiInt.sendMIDINBytesUART(packet, n_available);
    }
    else
    {
        ledMatrix.indicator_pattern(16, pulseCounter / (24), false);
    }

    clearPacket();
    n_available = midiInt.midiAvailableUART();
    if (n_available)
    {
        midiInt.getMIDIUART(packet);
        midiInt.sendMIDINBytesUSB(packet, n_available);
    }
}

void handle_start()
{
    pulseCounter = 0;
    MIDIIsLocked = false;
}

void handle_stop()
{
    ledMatrix.stop_pattern(AMBAR, true);
    pulseCounter = 0;
    MIDIIsLocked = true;
}

void handle_clock()
{
    if (MIDIIsLocked)
    {
        ledMatrix.stop_pattern(CYAN, false);
        return;
    }

    if (pulseCounter % 24 == 0)
    {
        ledMatrix.indicator_pattern(16, pulseCounter / (24), true);
    }

    pulseCounter = (pulseCounter + 1) % sequence_length;
}

void handle_note_on(uint8_t channel, uint8_t note, uint8_t velocity)
{

    ledMatrix.small_square(CYAN, true);
}

int main(void)
{
    board_init();

    stdio_init_all();

    midiInt.init();

    midiInt.initUART(TX_PIO, RX_PIO, TX_SM, RX_SM, PIN_TX, PIN_RX);
    ledMatrix.init(WS2812_PIN, LEDS_PIO, LEDS_SM);

    tud_init(BOARD_TUD_RHPORT);

    midiInt.onClockStart(&handle_start);
    midiInt.onClockStop(&handle_stop);
    midiInt.onClock(&handle_clock);
    midiInt.onNoteOn(&handle_note_on);

    ledMatrix.all_filled_pattern(AMBAR);

    while (1)
    {
        tud_task();
        midi_task();
    }
}