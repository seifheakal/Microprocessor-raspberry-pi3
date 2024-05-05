#include "lfb.h"
#include "uart.h"
#include "dbg.h"
#include "delays.h"

extern void gfx_beginframe();
extern void gfx_endframe();

extern void game_loop();

extern void update_pong();
extern void render_pong();

extern void fb_swap();
extern void delay_msec(int x);
extern void handle_pong_input(int x);
extern void fb_set_double_buffer(int x);

extern void composite_menu();
extern void mainloop_pong();

extern unsigned int getVal();

void kernel_secondary(int pid)
{
    wait_msec_st(3000 + 100 * pid);
    printf("Hello, from core %d\n", pid);

    uart_puts("hi");

    printf("Hello, from core %d\n", pid);

    while (1)
    {
        //asm volatile("wfe");
        char c = uart_getc();
        if (c == 'w') {
            handle_pong_input(-20);
        }
        else if (c == 's') {
            handle_pong_input(20);
        }

        uart_send(c);
    }
}

char handle_input() {
    char c = uart_getc();
    return c;
}

extern int gpio_input (unsigned int gpio);

int main() {
    uart_init();

    printf("Initialized UART\n");

    lfb_init();

    uart_puts("yoooooooooooooooooo again lol test123\n");

    // display a pixmap
    //lfb_showpicture();

    //render_pong();

    composite_menu();

    mainloop_pong();
    
    while (1) {
        uart_send(uart_getc());

        if (gpio_input(26)) {
            uart_puts("26\n");
        }

        if (gpio_input(16)) {
            uart_puts("16\n");
        }

        if (gpio_input(18)) {
            uart_puts("6\n");
        }

        if (gpio_input(24)) {
            uart_puts("5\n");
        }

        if (gpio_input(13)) {
            uart_puts("13\n");
        }

        if (gpio_input(12)) {
            uart_puts("12\n");
        }
    }

    return 0;
}