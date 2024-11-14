#include "hardware/pio.h"
#include "ws2812.pio.h"
#include "bsp/board.h"
#include "bsp/board_api.h"
#include "tusb.h"

#define AMBAR 0x0A3000
#define WHITE 0x121212
#define CYAN 0x150004

class LedMatrix
{
private:
    uint32_t current[16] = {0};
    uint32_t buffer[16] = {0};
    uint32_t idx = 0;
    PIO _pio;
    uint _sm;
    uint8_t pin;
    void put_pixel(uint32_t pixel_grb, bool overlay);
    void send_pixel(uint32_t value);
    void maybe_send();

public:
    void init(uint8_t _pin, PIO __pio, uint __sm);
    void indicator_pattern(uint len, uint t, bool overlay = false);
    void stop_pattern(int color, bool overlay = false);
    void all_filled_pattern(uint32_t color, bool overlay = false);
    void small_square(uint32_t color, bool overlay = false);
};
