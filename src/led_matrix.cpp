#include "led_matrix.h"

void LedMatrix::init(uint8_t _pin, PIO __pio, uint __sm)
{
    pin = _pin;
    _pio = __pio;
    _sm = __sm;

    gpio_set_drive_strength(pin, GPIO_DRIVE_STRENGTH_12MA);

    uint offset1 = pio_add_program(_pio, &ws2812_program);
    ws2812_program_init(_pio, _sm, offset1, _pin, 800000, false);
    gpio_set_drive_strength(_pin, GPIO_DRIVE_STRENGTH_12MA);
}

inline void LedMatrix::send_pixel(uint32_t pixel_grb)
{
    pio_sm_put_blocking(_pio, _sm, pixel_grb << 8u);
}

void LedMatrix::put_pixel(uint32_t pixel_grb, bool overlay)
{
    if (overlay)
    {
        buffer[idx] = pixel_grb ? pixel_grb : current[idx];
    }
    else
    {
        buffer[idx] = pixel_grb;
    }
    idx = (idx + 1) % 16;

    if (idx == 0)
    {
        maybe_send();
    }
}

void LedMatrix::maybe_send()
{
    bool has_changed = false;
    for (int i = 0; i < 16; i++)
    {
        if (current[i] != buffer[i])
        {
            has_changed = true;
            break;
        }
    }
    if (has_changed)
    {
        for (int i = 0; i < 16; i++)
        {
            send_pixel(buffer[i]);
            current[i] = buffer[i];
        }
    }
}

void LedMatrix::indicator_pattern(uint len, uint t, bool overlay)
{
    for (int i = 0; i < len; ++i)
    {
        if (i == t)
        {
            if (i == 0 || i == 12)
            {
                put_pixel(WHITE, overlay);
            }
            else
            {
                put_pixel(AMBAR, overlay);
            }
        }
        else
        {
            put_pixel(0, overlay);
        }
    }
}

void LedMatrix::stop_pattern(int color, bool overlay)
{
    for (int i = 0; i < 4; ++i)
    {
        put_pixel(color, overlay);
    }
    for (int i = 0; i < 2; ++i)
    {
        put_pixel(color, overlay);

        put_pixel(0, overlay);
        put_pixel(0, overlay);
        put_pixel(color, overlay);
    }
    for (int i = 0; i < 4; ++i)
    {
        put_pixel(color, overlay);
    }
}

void LedMatrix::all_filled_pattern(uint32_t color, bool overlay)
{
    if (color)
        printf("fill %08X\n", color);
    for (int i = 0; i < 16; ++i)
    {
        put_pixel(color, overlay);
    }
}

void LedMatrix::small_square(uint32_t color, bool overlay)
{
    for (int i = 0; i < 16; ++i)
    {
        if (i == 5 || i == 6 || i == 9 || i == 10)
        {
            put_pixel(color, overlay);
        }
        else
        {
            put_pixel(0, overlay);
        }
    }
}