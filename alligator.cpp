#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "hardware/pio.h"
#include "hardware/clocks.h"

#include "midi.h"
#include "ws2812.pio.h"

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

#define AMBAR 0x0A3000
#define WHITE 0x121212

void clearPacket()
{
    for (int i = 0; i < 128; i++)
    {
        packet[i] = 0;
    }
}

void midi_task()
{
    clearPacket();
    int n_available = midiInt.midiAvailableUSB();
    if (n_available > 0)
    {
        midiInt.getMIDIUSB(packet);
        midiInt.sendMIDINBytesUART(packet, n_available);
    }
    midiInt.update();

    clearPacket();
    n_available = midiInt.midiAvailableUART();
    if (n_available)
    {
        midiInt.getMIDIUART(packet);
        midiInt.sendMIDINBytesUSB(packet, n_available);
    }
}

static inline void put_pixel(uint32_t pixel_grb)
{
    pio_sm_put_blocking(LEDS_PIO, LEDS_SM, pixel_grb << 8u);
}

void indicator_pattern(uint len, uint t)
{
    for (int i = 0; i < len; ++i)
    {
        if (i == t)
        {
            if (i == 0 || i == 12)
            {
                put_pixel(WHITE);
            }
            else
            {
                put_pixel(AMBAR);
            }
        }
        else
        {
            put_pixel(0);
        }
    }
}

void stop_pattern(uint len)
{
    for (int i = 0; i < 4; ++i)
    {
        put_pixel(AMBAR);
    }
    for (int i = 0; i < 2; ++i)
    {
        put_pixel(AMBAR);

        put_pixel(0);
        put_pixel(0);
        put_pixel(AMBAR);
    }
    for (int i = 0; i < 4; ++i)
    {
        put_pixel(AMBAR);
    }
}

void handle_start()
{
    pulseCounter = 0;
    MIDIIsLocked = false;
}

void handle_stop()
{
    stop_pattern(16);
    pulseCounter = 0;
    MIDIIsLocked = true;
}

void handle_clock()
{
    if (MIDIIsLocked)
    {
        return;
    }

    if (pulseCounter % 24 == 0)
    {
        indicator_pattern(16, pulseCounter / (24));
    }

    pulseCounter = (pulseCounter + 1) % sequence_length;
}

void init_ws2812()
{
    gpio_set_drive_strength(WS2812_PIN, GPIO_DRIVE_STRENGTH_12MA);

    uint offset1 = pio_add_program(LEDS_PIO, &ws2812_program);
    ws2812_program_init(LEDS_PIO, LEDS_SM, offset1, WS2812_PIN, 800000, false);
    gpio_set_drive_strength(WS2812_PIN, GPIO_DRIVE_STRENGTH_12MA);
}

int main(void)
{
    board_init();

    stdio_init_all();

    midiInt.init();

    midiInt.initUART(TX_PIO, RX_PIO, TX_SM, RX_SM, PIN_TX, PIN_RX);
    init_ws2812();

    tud_init(BOARD_TUD_RHPORT);

    midiInt.onClockStart(&handle_start);
    midiInt.onClockStop(&handle_stop);
    midiInt.onClock(&handle_clock);
    stop_pattern(16);

    while (1)
    {
        tud_task();
        midi_task();
    }
}