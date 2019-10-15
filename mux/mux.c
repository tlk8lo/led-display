#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <inttypes.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#ifndef UART_BAUD
#define UART_BAUD 9600
#endif

#ifndef OSC_CALIBRATION
#define OSC_CALIBRATION OSCCAL
#endif

/*
 * Display buffers
 */
#define DISP_SIZE 4
#define DISP_LEN (DISP_SIZE * DISP_SIZE * 3)
volatile unsigned char buf_a[DISP_SIZE * DISP_SIZE * 3] = {0};
volatile unsigned char buf_b[DISP_SIZE * DISP_SIZE * 3] = {0};
volatile unsigned char *buf_front = buf_a, *cursor = buf_a, *buf_back = buf_a, *disp = buf_a;
volatile unsigned char buf_dbl = 0;

/*
 * Clear display command
 */
void cls( )
{
	for ( int i = 0; i < DISP_LEN; i++ )
		buf_back[i] = 0;
}

/*
 * USART receive complete interrupt
 */
ISR( USART_RX_vect )
{
	//Process incoming data
	uint8_t b = UDR;
	
	if ( b & 0x80 ) //Command
	{
		//Bits 0-2 determine command
		//Bits 3-5 are command parameter
		//Bit 7 is frame type
		uint8_t command = b & 0b00000111;
		uint8_t param = ( b & 0b01111000 ) >> 3; 
		switch ( command )
		{
			//0 - CBK
			case 0:
				cursor = disp;
				break;

			//1 - CLS
			case 1:
				cls( );
				break;

			//2 - BCL
			case 2:
				if ( param )
				{
					buf_back = cursor = buf_a;
					buf_front = disp = buf_b;
					buf_dbl = 1;
				}
				else
				{
					buf_back = cursor = buf_front = disp = buf_a;
					buf_dbl = 0;
				}
				break;

			//3 - BSW
			case 3:
				if ( buf_dbl )
				{
					//Swap buffer base pointers
					volatile unsigned char *tmp = buf_front;
					buf_front = buf_back;
					buf_back = tmp;

					//Add offsets
					cursor = buf_back; 
					disp = buf_front;
				}
				break;
		}
	}
	else //Data
	{
		//Move cursor and write data
		*cursor = b * 2;
		if ( ++cursor - buf_back > DISP_SIZE * DISP_SIZE * 3 ) cursor = buf_back;
	}
}

/*
 * Basic UART config function
 * 8 bit data, 1 bit stop
 * RX Interrupt enabled
 * TX disabled
 */
void uart_config( unsigned int baud )
{
	baud = F_CPU / 16 / baud - 1;
	UBRRL = (unsigned char) baud;
	UBRRH = (unsigned char)( baud >> 8 );
	UCSRA = 0;
	UCSRB = ( 1 << RXCIE ) | ( 1 << RXEN );
	UCSRC = ( 1 << UCSZ0 ) | ( 1 << UCSZ1 );
}

/*
 * Display multiplexer
 *
 * COLUMNS:
 * 	B0 - R3
 * 	B1 - G3
 * 	B2 - B3
 *
 * 	B3 - R2
 * 	B4 - G2
 * 	B5 - B2
 * 	
 * 	B6 - R1
 * 	B7 - G1
 *	A0 - B1
 *
 *	A1 - R0
 *	D1 - G0
 *	D6 - B0
 *
*/
void mux( )
{
	#define LED(port, pin, pwm, cnt) \
		{\
			if( (pwm) > (cnt) ) \
				(port) |= ( 1 << (pin) );\
			else\
				(port) &= ~( 1 << (pin) );\
		}

	//PWM
	for ( int i = 0; i < 256; i += 8 )
	{
		//Row
		for ( int j = 0; j < 4; j++ )
		{
			//Clear rows
			PORTD = 0;	
			_delay_us( 5 );

			//Colums 0 and 1
			LED( PORTA, 1, disp[j * 12 +  0], i ); //R0 
			LED( PORTD, 1, disp[j * 12 +  1], i ); //G0
			LED( PORTD, 6, disp[j * 12 +  2], i ); //B0
						
			LED( PORTB, 6, disp[j * 12 +  3], i ); //R1 
			LED( PORTB, 7, disp[j * 12 +  4], i ); //G1
			LED( PORTA, 0, disp[j * 12 +  5], i ); //B1
			
			//Columns 2 and 3
			LED( PORTB, 3, disp[j * 12 +  6], i ); //R0 
			LED( PORTB, 4, disp[j * 12 +  7], i ); //G0
			LED( PORTB, 5, disp[j * 12 +  8], i ); //B0
						
			LED( PORTB, 0, disp[j * 12 +  9], i ); //R1 
			LED( PORTB, 1, disp[j * 12 + 10], i ); //G1
			LED( PORTB, 2, disp[j * 12 + 11], i ); //B1	
	

			//Select row and wait
			PORTD |= ( 1 << ( j + 2 ) );
			_delay_us( 50 );
		}
	}

	#undef LED
}

/*
 */
int main( )
{
	//Calibrate oscillator
	OSCCAL = OSC_CALIBRATION;
	_delay_ms( 5 );
	
	//UART config
	uart_config( UART_BAUD );

	//Interrupts enabled
	sei( );

	//DDRs
	DDRB |= 0xFF;
	DDRA |= 0x03;
	DDRD |= 0xFE;

	//Clear display
	cls( );

	//The main loop
	while ( 1 )
		mux( );

	return 0;
}
