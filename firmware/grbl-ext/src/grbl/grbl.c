#include "grbl.h"

void grbl_run(hal_interface_t *hal)
{
    // Loop forever (GRBL never exits)
    while (true)
    {
        uint8_t c;
        while ((c = hal->terminal.char_recv()) != 0)
        {
            while (hal->terminal.can_send())
                ;

            hal->terminal.char_send(c);
        }

        hal->timer.delay_ms(10);
    }
}