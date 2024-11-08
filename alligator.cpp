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
#define RX_PIO pio0
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

// void handle_clock(uint8_t command);

void clearPacket()
{
    for (int i = 0; i < 128; i++)
    {
        packet[i] = 0;
    }
}

void midi_task()
{
    clearPacket(); // maybe not needed
    int n_available = midiInt.midiAvailableUSB();
    if (n_available > 0)
    {
        printf("%d MIDI:\n", n_available);
        midiInt.getMIDIUSB(packet);
        for (int i = 0; i < n_available; i++)
        {
            printf("    %02X ", packet[i]);
        }
        printf("\n");

        // if (packet[0] == CLOCK_SIGNAL || packet[0] == CLOCK_START || packet[0] == CLOCK_STOP)
        // {
        //     handle_clock(packet[0]);
        // }
        // midiInt.sendMIDIStatusUART(packet[0], packet[1], packet[2]);

        // midiInt.sendMIDINBytesUART(packet, n_available);
    }
    // midiInt.update();

    // clearPacket();
    // if (midiInt.midiAvailableUART())
    // {
    //     midiInt.getMIDIUART(packet);
    //     midiInt.sendMIDIStatusUSB(packet[0], packet[1], packet[2]);
    // }
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

// void handle_clock(uint8_t command)
// {
//     if (command == CLOCK_START)
//     {
//         pulseCounter = 0;
//         MIDIIsLocked = false;
//         return;
//     }
//     if (command == CLOCK_STOP)
//     {
//         stop_pattern(16);
//         pulseCounter = 0;
//         MIDIIsLocked = true;
//         return;
//     }
//     if (MIDIIsLocked)
//     {
//         return;
//     }

//     if (pulseCounter % 24 == 0)
//     {
//         indicator_pattern(16, pulseCounter / (24));
//     }

//     pulseCounter = (pulseCounter + 1) % sequence_length;
// }

void serial_task()
{
    static uint32_t last = 0;
    if (millis() - last > 1000)
    {
        printf("Hello World\n");
        last = millis();
    }
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

    // midiInt.init();

    midiInt.initUART(TX_PIO, RX_PIO, TX_SM, RX_SM, PIN_TX, PIN_RX);
    init_ws2812();

    tud_init(BOARD_TUD_RHPORT);

    // midiInt.onClockStart(&handle_start);
    // midiInt.onClockStop(&handle_stop);
    // midiInt.onClock(&handle_clock);

    stop_pattern(16);
    while (1)
    {
        tud_task();
        midi_task();
        serial_task();
    }
}