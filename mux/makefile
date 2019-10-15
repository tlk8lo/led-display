CFLAGS = -Wall -mmcu=attiny2313 -Os --std=c99
CC = avr-gcc
F_CPU = 8000000UL
OSC_CALIBRATION = 80
UART_BAUD = 19200 #9600

all:
	$(CC) $(CFLAGS) -DF_CPU=$(F_CPU) -DUART_BAUD=$(UART_BAUD) -DOSC_CALIBRATION=$(OSC_CALIBRATION) -o mux.elf mux.c

prog:
	avrdude -c usbasp -p t2313 -U flash:w:mux.elf
	
