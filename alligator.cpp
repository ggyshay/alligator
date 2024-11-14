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
#define CYAN 0x300030

void clearPacket()
{
    for (int i = 0; i < 128; i++)
    {
        packet[i] = 0;
    }
}

void all_filled_pattern(uint32_t color);

void midi_task()
{
    midiInt.update();
    clearPacket();
    int n_available = midiInt.midiAvailableUSB();
    if (n_available > 0)
    {
        // midiInt.clean_buffer.print();
        // printf("MIDI available: %d\n", n_available);
        // printf("usb: ");
        midiInt.getMIDIUSB(packet);
        // printf("\n");
        // for (int i = 0; i < n_available; i++)
        // {
        //     printf("%02X ", packet[i]);
        // }
        // printf("\n");
        midiInt.sendMIDINBytesUART(packet, n_available);
    }
    // midiInt.update();

    clearPacket();
    n_available = midiInt.midiAvailableUART();
    if (n_available)
    {
        midiInt.getMIDIUART(packet);
        // printf("uart (%d): ", n_available);
        // midiInt.uart_buffer.print();
        // for (int i = 0; i < n_available; i++)
        // {
        //     printf("%02X ", packet[i]);
        // }
        // printf("\n");
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

void stop_pattern(int color)
{
    for (int i = 0; i < 4; ++i)
    {
        put_pixel(color);
    }
    for (int i = 0; i < 2; ++i)
    {
        put_pixel(color);

        put_pixel(0);
        put_pixel(0);
        put_pixel(color);
    }
    for (int i = 0; i < 4; ++i)
    {
        put_pixel(color);
    }
}

void all_filled_pattern(uint32_t color)
{
    static uint32_t last = 0;
    if (color == last)
    {
        return;
    }
    last = color;
    for (int i = 0; i < 16; ++i)
    {
        put_pixel(color);
    }
}

void handle_start()
{
    pulseCounter = 0;
    MIDIIsLocked = false;
}

void handle_stop()
{
    stop_pattern(AMBAR);
    pulseCounter = 0;
    MIDIIsLocked = true;
}

void handle_clock()
{
    if (MIDIIsLocked)
    {
        stop_pattern(CYAN);
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

void cron_task()
{
    static uint32_t last = 0;
    static bool version = false;
    uint8_t pack0[] = {
        0x90,
        36,
        127};

    uint8_t pack1[] = {
        0xB0,
        0,
        0x01,
        0xB0,
        0x20,
        0x02,
        0xC0,
        0x02,
    };

    if (millis() - last > 1000)
    {
        last = millis();
        uint8_t note_on[3] = {0x90, 36, 127};
        tud_midi_stream_write(0, note_on, 3);
        // midiInt.sendMIDINBytesUSB(pack0, 3);
        // printf("hello\n");

        // if (version)
        // {
        //     midiInt.sendMIDINBytesUART(pack0, 8);
        // }
        // else
        // {
        //     midiInt.sendMIDINBytesUART(pack1, 8);
        // }
        // version = !version;
    }
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
    // stop_pattern(16);
    all_filled_pattern(AMBAR);

    while (1)
    {
        tud_task();
        midi_task();
        // cron_task();
    }
}