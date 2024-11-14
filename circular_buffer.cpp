#include "circular_buffer.h"

void CircularBuffer::write(uint8_t *packet, int n)
{
    for (int i = 0; i < n; i++)
    {
        buffer[write_ptr] = packet[i];
        write_ptr = (write_ptr + 1) % 128;
    }
}

void CircularBuffer::write(uint8_t byte)
{
    buffer[write_ptr] = byte;
    write_ptr = (write_ptr + 1) % 128;
}

void CircularBuffer::read(uint8_t *packet, int n)
{
    for (int i = 0; i < n; i++)
    {
        packet[i] = buffer[read_ptr];
        read_ptr = (read_ptr + 1) % 128;
    }
}

uint8_t CircularBuffer::read()
{
    uint8_t r = buffer[read_ptr];
    read_ptr = (read_ptr + 1) % 128;
    return r;
}

uint8_t CircularBuffer::peak()
{
    return buffer[read_ptr];
}

uint8_t CircularBuffer::peak(int n)
{
    return buffer[(read_ptr + n) % 128];
}

int CircularBuffer::available()
{
    return (write_ptr - read_ptr + 128) % 128;
}

void CircularBuffer::get_from_buffer(CircularBuffer *buff, int n)
{
    for (int i = 0; i < n; i++)
    {
        write(buff->read());
    }
}

void CircularBuffer::clear()
{
    write_ptr = 0;
    read_ptr = 0;
}

void CircularBuffer::print()
{
    int i = read_ptr;
    while (i != write_ptr)
    {
        printf("%02X ", buffer[i]);
        i = (i + 1) % 128;
    }
    printf("\n");
}