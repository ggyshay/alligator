#include "pico/stdlib.h"

#include "bsp/board.h"
#include "bsp/board_api.h"
#include "tusb.h"

#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H
class CircularBuffer
{
private:
    uint8_t buffer[128];
    uint8_t write_ptr = 0;
    uint8_t read_ptr = 0;

public:
    void write(uint8_t *packet, int n);
    void write(uint8_t byte);
    void read(uint8_t *packet, int n);
    uint8_t read();
    uint8_t peak();
    uint8_t peak(int n);
    int available();
    void get_from_buffer(CircularBuffer *buff, int n);
    void clear();
    void print();
};
#endif