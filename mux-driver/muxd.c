// HAHAHAHAHAH
#ifdef NDEBUG
#undef NDEBUG
#warning "Nobody ain't taking my asserions away!"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>
#include <assert.h>
#include <signal.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

int alive = 1;
int serialfd = -1;

const int bright[8] = {0, 1, 8, 16, 32, 64, 128, 255};

/*
 * Serial setup routine
 */
int init_serial( const char *fname )
{
	int fd = open( fname, O_RDWR | O_NOCTTY | O_SYNC );
	assert( fd != -1 );

	struct termios tty;
	memset( &tty, 0, sizeof tty );
	assert( tcgetattr( fd, &tty ) == 0 );
	//cfsetospeed( &tty, B9600 );
	//cfsetispeed( &tty, B9600 );
	
	cfsetospeed( &tty, B19200 );
	cfsetispeed( &tty, B19200 );

	tty.c_cflag = ( tty.c_cflag & ~CSIZE ) | CS8;
	tty.c_iflag &= ~IGNBRK;
	tty.c_lflag = 0;
	tty.c_oflag = 0;
	tty.c_cc[VMIN] = 0;
	tty.c_cc[VTIME] = 5;
	tty.c_iflag &= ~( IXON | IXOFF | IXANY );
	tty.c_cflag |= ( CLOCAL | CREAD );
	tty.c_cflag &= ~( PARENB | PARODD );
	//tty.c_cflag |= parity;
	tty.c_cflag &= ~CSTOPB;
	tty.c_cflag &= ~CRTSCTS;
	
	assert( tcsetattr( fd, TCSANOW, &tty ) == 0 );

	return fd;
}

/*
 * Send LED color data
 */
void led( unsigned char r, unsigned char g, unsigned char b )
{
	unsigned char data[3];
	data[0] = r / 2;
	data[1] = g / 2;
	data[2] = b / 2;
	write( serialfd, data, 3 );
}

/*
 * Send display control command
 */
#define DISP_CBK 0 //Move cursor back
#define DISP_CLS 1 //Clear display
#define DISP_BCL 2 //Buffer control
#define DISP_BCL_ON 19 //Double buffering ON
#define DISP_BCL_OFF 3 //Double buffering OFF
#define DISP_BSW 3 //Buffer swap
void dispcmd( int cmd )
{
	unsigned char cmdb = 0x80 | ( cmd & 0x7f );
	write( serialfd, &cmdb, 1 );
}


/*
 * Display data
 */
#define DISP_SIZE 4
struct rgb
{
	unsigned char r, g, b;
} __attribute__( ( packed ) );
struct rgb disp[DISP_SIZE][DISP_SIZE] = {{{0}}};

/*
 * Display data transmission function
 */
void dispflush( )
{
	dispcmd( DISP_CBK );
	
	static unsigned char data[DISP_SIZE * DISP_SIZE * 3];
	memcpy( data, disp, sizeof data );

	int i;
	for ( i = 0; i < sizeof data; i++ )
		data[i] /= 2;

	write( serialfd, data, sizeof data );
	dispcmd( DISP_BSW );
}

/*
 * Exit signal handler
 */
void sighndl( int signum )
{
	fprintf( stderr, "SIGINT received\n" );
	close( serialfd );
	exit( 0 );
}

int main( int argc, const char *argv[] )
{
	if ( argc < 3 )
	{
		perror( "Usage: ./muxd <serial> <image> [delay]\n" );
		exit( EXIT_FAILURE );
	}

	float frame_delay = 500.0;
	if ( argc >= 4 ) assert( sscanf( argv[3], "%f", &frame_delay ) );
	assert( frame_delay >= 30.0 && frame_delay < 1000.0 );

	//Init serial
	serialfd = init_serial( argv[1] );

	//Init signal handler
	signal( SIGINT, sighndl );

	//Read PNG file
	int image_width, image_height, image_bpp, frame_count;
	unsigned char *image_data = stbi_load( argv[2], &image_width, &image_height, &image_bpp, 3 ); // force 3 bytes per pixel
	assert( image_width == 4 && image_height % 4 == 0 && "bad dimensions..." );
	assert( image_data && "stbi_load() failed" );

	frame_count = image_height / 4;

	//Reset the display
	dispcmd( DISP_BCL_ON );
	dispcmd( DISP_CBK );
	dispcmd( DISP_CLS );

	//Main animation loop
	while ( alive )
	{
		for ( int i = 0; i < frame_count; i++ )
		{
			memcpy( disp, image_data + i * 16 * 3, 16 * 3 ); 
			dispflush( );
				
			// Frame delay
			struct timespec delay = 
			{
				.tv_sec = 0,
				.tv_nsec = 1e6 * frame_delay
			};
			nanosleep( &delay, NULL );
		}
	}

	// Free image data
	stbi_image_free( image_data );

	return 0;
}
