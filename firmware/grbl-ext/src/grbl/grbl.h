#ifndef __GRBL_H__
#define __GRBL_H__

#include <stdint.h>
#include <stdbool.h>

typedef struct
{
    // Receive a single character (blocking)
    uint8_t (*char_recv)();

    // Send a single character (blocking)
    void (*char_send)(uint8_t c);

    // Can read a character from receive buffer
    bool (*can_recv)();

    // Can add a character to send buffer
    bool (*can_send)();
} terminal_interface_t;

typedef struct
{
    void (*delay_ms)(uint32_t ms);
} timer_interface_t;

typedef struct
{
    terminal_interface_t terminal;
    timer_interface_t timer;

    // The ability to enter and exit critical sections (e.g. HAL may disable and reenable interrupts)
    void (*enter_critical)();
    void (*exit_critical)();
} hal_interface_t;

void grbl_run(hal_interface_t *grbl);

#endif // __GRBL_H__