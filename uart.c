/*
 * Copyright (C) 2018 bzt (bztsrc@github)
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

#include "gpio.h"
#include "mbox.h"
#include "delays.h"
#include "sprintf.h"

/* PL011 UART registers */
#define UART0_DR        ((volatile unsigned int*)(MMIO_BASE+0x00201000))
#define UART0_FR        ((volatile unsigned int*)(MMIO_BASE+0x00201018))
#define UART0_IBRD      ((volatile unsigned int*)(MMIO_BASE+0x00201024))
#define UART0_FBRD      ((volatile unsigned int*)(MMIO_BASE+0x00201028))
#define UART0_LCRH      ((volatile unsigned int*)(MMIO_BASE+0x0020102C))
#define UART0_CR        ((volatile unsigned int*)(MMIO_BASE+0x00201030))
#define UART0_IMSC      ((volatile unsigned int*)(MMIO_BASE+0x00201038))
#define UART0_ICR       ((volatile unsigned int*)(MMIO_BASE+0x00201044))

#define AUX_ENABLE      ((volatile unsigned int*)(MMIO_BASE+0x00215004))
#define AUX_MU_IO       ((volatile unsigned int*)(MMIO_BASE+0x00215040))
#define AUX_MU_IER      ((volatile unsigned int*)(MMIO_BASE+0x00215044))
#define AUX_MU_IIR      ((volatile unsigned int*)(MMIO_BASE+0x00215048))
#define AUX_MU_LCR      ((volatile unsigned int*)(MMIO_BASE+0x0021504C))
#define AUX_MU_MCR      ((volatile unsigned int*)(MMIO_BASE+0x00215050))
#define AUX_MU_LSR      ((volatile unsigned int*)(MMIO_BASE+0x00215054))
#define AUX_MU_MSR      ((volatile unsigned int*)(MMIO_BASE+0x00215058))
#define AUX_MU_SCRATCH  ((volatile unsigned int*)(MMIO_BASE+0x0021505C))
#define AUX_MU_CNTL     ((volatile unsigned int*)(MMIO_BASE+0x00215060))
#define AUX_MU_STAT     ((volatile unsigned int*)(MMIO_BASE+0x00215064))
#define AUX_MU_BAUD     ((volatile unsigned int*)(MMIO_BASE+0x00215068))

extern volatile unsigned char _end;

#define mbox mbox_buffer

enum GPIOMODE {
	GPIO_INPUT = 0b000,									// 0
	GPIO_OUTPUT = 0b001,								// 1
	GPIO_ALTFUNC5 = 0b010,								// 2
	GPIO_ALTFUNC4 = 0b011,								// 3
	GPIO_ALTFUNC0 = 0b100,								// 4
	GPIO_ALTFUNC1 = 0b101,								// 5
	GPIO_ALTFUNC2 = 0b110,								// 6
	GPIO_ALTFUNC3 = 0b111,								// 7
};

#define PI_IOBASE_ADDR 0x3F000000    // On a Pi3 change to 0x3F000000

int gpio_setup (unsigned int gpio, unsigned int mode) 
{
    unsigned int* GPFSEL  = (unsigned int*) (PI_IOBASE_ADDR + 0x200000 + 0x0);
	if (gpio > 54) return 0;		// Check GPIO pin number valid, return false if invalid
	if (mode < 0 || mode > GPIO_ALTFUNC3) return 0;	// Check requested mode is valid, return false if invalid
	unsigned int bit = ((gpio % 10) * 3);	// Create bit mask
	unsigned int mem = GPFSEL[gpio / 10];	// Read register
	mem &= ~(7 << bit);		// Clear GPIO mode bits for that port
	mem |= (mode << bit);		// Logical OR GPIO mode bits
	GPFSEL[gpio / 10] = mem;	 // Write value to register
	return 1;	// Return true
}

int gpio_input (unsigned int gpio) 
{
    unsigned int* GPLEV  = (unsigned int*) (PI_IOBASE_ADDR + 0x200000 + 0x34);
	if (gpio < 54)	// Check GPIO pin number valid, return false if invalid
	{
		unsigned int bit = 1 << (gpio % 32);	// Create mask bit
		unsigned int  mem = GPLEV[gpio / 32];	// Read port level
		if (mem & bit) return 1;	// Return true if bit set
	}
	return 0;	// Return false
}

/**
 * Set baud rate and characteristics (115200 8N1) and map to GPIO
 */
void uart_init()
{
    register unsigned int r;

    /* initialize UART */
    *AUX_ENABLE |=1;       // enable UART1, AUX mini uart
    *AUX_MU_CNTL = 0;
    *AUX_MU_LCR = 3;       // 8 bits
    *AUX_MU_MCR = 0;
    *AUX_MU_IER = 0;
    *AUX_MU_IIR = 0xc6;    // disable interrupts
    *AUX_MU_BAUD = 3254; //270;    // 115200 baud

    /* map UART1 to GPIO pins */
    r=*GPFSEL1;
    r&=~((7<<12)|(7<<15)); // gpio14, gpio15
    r|=(2<<12)|(2<<15);    // alt5

    *GPFSEL1 = r;

    *GPPUD = 0;            // enable pins 14 and 15
    r=150; while(r--) { asm volatile("nop"); }
    *GPPUDCLK0 = (1<<14)|(1<<15);
    r=150; while(r--) { asm volatile("nop"); }
    *GPPUDCLK0 = 0;        // flush GPIO setup
    *AUX_MU_CNTL = 3;      // enable Tx, Rx

    // register unsigned int r;

    // /* initialize UART */
    // *UART0_CR = 0;         // turn off UART0

    // /* set up clock for consistent divisor values */
    // mbox[0] = 9*4;
    // mbox[1] = MBOX_REQUEST;
    // mbox[2] = MBOX_TAG_SETCLKRATE; // set clock rate
    // mbox[3] = 12;
    // mbox[4] = 8;
    // mbox[5] = 2;           // UART clock
    // mbox[6] = 4000000;     // 4Mhz
    // mbox[7] = 0;           // clear turbo
    // mbox[8] = MBOX_TAG_LAST;
    // mbox_callLOC(MBOX_CH_PROP);

    // /* map UART0 to GPIO pins */
    // r=*GPFSEL1;
    // r&=~((7<<12)|(7<<15)); // gpio14, gpio15
    // r|=(4<<12)|(4<<15);    // alt0
    // *GPFSEL1 = r;
    // *GPPUD = 0;            // enable pins 14 and 15
    // wait_cycles(150);
    // *GPPUDCLK0 = (1<<14)|(1<<15);
    // wait_cycles(150);
    // *GPPUDCLK0 = 0;        // flush GPIO setup

    // *UART0_ICR = 0x7FF;    // clear interrupts
    // *UART0_IBRD = 2;       // 115200 baud
    // *UART0_FBRD = 0xB;
    // *UART0_LCRH = 0x7<<4;  // 8n1, enable FIFOs
    // *UART0_CR = 0x301;     // enable Tx, Rx, UART

    gpio_setup(26, GPIO_INPUT);
    gpio_setup(16, GPIO_INPUT);
    gpio_setup(18, GPIO_INPUT);
    gpio_setup(24, GPIO_INPUT);
    gpio_setup(13, GPIO_INPUT);
    gpio_setup(12, GPIO_INPUT);
}


unsigned int getVal() {
    unsigned int r = *GPLEV0;
    return (r >> 26) & 0x1; 
}

/**
 * Send a character
 */
void uart_send(unsigned int c) {
    /* wait until we can send */
    do{asm volatile("nop");}while(!(*AUX_MU_LSR&0x20));
    /* write the character to the buffer */
    *AUX_MU_IO=c;
}

/**
 * Receive a character
 */
char uart_getc() {
    char r;
    /* wait until something is in the buffer */
    do{asm volatile("nop");}while(!(*AUX_MU_LSR&0x01));
    /* read it and return */
    r=(char)(*AUX_MU_IO);
    /* convert carriage return to newline */
    return r=='\r'?'\n':r;
}

int hasChar() {
    return *AUX_MU_LSR&0x01;
}

char getChar() {
    return (char)(*AUX_MU_IO);
}

/**
 * Display a string
 */
void uart_puts(char *s) {
    while(*s) {
        /* convert newline to carriage return + newline */
        if(*s=='\n')
            uart_send('\r');
        uart_send(*s++);
    }
}

void printf(char *fmt, ...);

void LOLXDD() {
    unsigned int value_w1;
    asm volatile ("mov %0, x20" : "=r" (value_w1));
    printf("x20: %x\n", value_w1);
}

void LOLX() {
    uart_puts("mrk\n");
}

void Error(int where) {
    printf("Error at: %d\n", where);
}

void ASMDBG(int step) {
    printf("Assembly Step [%d]\n", step);
}

void PrintInt(unsigned int value) {
    printf("Value: 0x%x\n", value);
}

void PrintLong(unsigned long value) {
    printf("LValue: 0x%x\n", value);
}

void PrintChar(char c) {
    printf("C: %c\n", c);
}

void PrintBuffer(unsigned int* buf, int size) {
    for (int i = 0; i < size; i++) {
        printf("[%d] Value: 0x%x\n", buf[i]);
    }
}

/**
 * Display a binary value in hexadecimal
 */
void uart_hex(unsigned int d) {
    unsigned int n;
    int c;
    for(c=28;c>=0;c-=4) {
        // get highest tetrad
        n=(d>>c)&0xF;
        // 0-9 => '0'-'9', 10-15 => 'A'-'F'
        n+=n>9?0x37:0x30;
        uart_send(n);
    }
}

void printf(char *fmt, ...) {
    __builtin_va_list args;
    __builtin_va_start(args, fmt);
    // we don't have memory allocation yet, so we
    // simply place our string after our code
    char *s = (char*)&_end;
    // use sprintf to format our string
    vsprintf(s,fmt,args);
    // print out as usual
    while(*s) {
        /* convert newline to carrige return + newline */
        if(*s=='\n')
            uart_send('\r');
        uart_send(*s++);
    }
}