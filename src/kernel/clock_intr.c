#include <kernel/asm.h>
#include <kernel/clock_intr.h>

void set_8253_freq(uint16_t freq)
{
    uint16_t value = 1193180 / freq;

    _out_byte_to_port(0x43, (3 << 4) | (2 << 1));
    _out_byte_to_port(0x40, (uint8_t)value);
    _out_byte_to_port(0x40, (uint8_t)(value >> 8));
}
