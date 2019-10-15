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
#include <libpng16/png.h>

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

/**
	Reads PNG file into array
*/
unsigned char *slurp_png( const char *filename, int *width, int *height, int *bpp )
{
	unsigned char *data = NULL;

	assert( width && height && bpp );

	FILE *f = fopen( filename, "rb" );
	assert( f != NULL );
	
	png_structp png = png_create_read_struct( PNG_LIBPNG_VER_STRING, NULL, NULL, NULL );
	assert( png != NULL );
	
	png_infop info = png_create_info_struct( png );
	assert( info != NULL );
	
	if ( setjmp( png_jmpbuf( png ) ) )
		assert( 0 && "PNG called longjmp()" );
		
	png_init_io( png, f );
	png_read_info( png, info );
	*width = png_get_image_width( png, info );
	*height = png_get_image_height( png, info );
	
	if ( png_get_valid( png, info, PNG_INFO_tRNS ) )
		png_set_tRNS_to_alpha( png );
		
	char has_alpha = 0;
	switch ( png_get_color_type( png, info ) )
	{
		case PNG_COLOR_TYPE_GRAY:
			assert( 0 && "fuck that" );
			png_set_gray_to_rgb( png );
			has_alpha = 0;
			break;
		
		case PNG_COLOR_TYPE_GRAY_ALPHA:
			assert( 0 && "fuck that" );
			png_set_gray_to_rgb( png );
			has_alpha = 1;
			break;
			
		case PNG_COLOR_TYPE_PALETTE:
			//assert( 0 && "fuck palletes!" );
			png_set_expand( png );
			has_alpha = 0;
			break;
			
		case PNG_COLOR_TYPE_RGBA:
			has_alpha = 1;
			break;
			
		default:
			assert( 0 && "Unhandled PNG type" );
			break;
	}
	
	png_set_interlace_handling( png );
	png_read_update_info( png, info );
	
	*bpp = has_alpha ? 4 : 3;
	
	//assert( !has_alpha && "PNG has alpha channel and I'm fucking lazy" );
	
	data = calloc( *bpp * *width * *height, sizeof( unsigned char ) );
	assert( data && "calloc() failed on `data`" );
	
	
	png_bytep *rows = calloc( *height, sizeof( png_bytep ) );
	assert( rows && "calloc() failed on `data`" );
	unsigned char *p = data;
	int i;
	for ( i = 0; i < *height; i++ )
	{
		rows[i] = p;
		p += *width * *bpp;
	}
	
	png_read_image( png, rows );
	
	free( rows );
	png_read_end( png, NULL );
	png_destroy_info_struct( png, &info );
	png_destroy_read_struct( &png, &info, NULL );
	fclose( f );
	 
	
	if ( data ) printf( "Done reading %dx%d PNG\n", *width, *height );
	return data;
}

unsigned char *discard_alpha( unsigned char *image, int size )
{
	assert( image && size );
	unsigned char *data = calloc( size * 3.f / 4.f, 1 );
	assert( data );
	
	int i;
	unsigned char *p = data;
	for ( i = 0; i < size; i++ )
	{
		if ( i % 3 )
		{
			*p++ = data[i];
		}
	}
	
	return data;
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
	unsigned char *image_data = slurp_png( argv[2], &image_width, &image_height, &image_bpp );
	//unsigned char *image_data = discard_alpha( png_data, image_width * image_height * image_bpp );
	//if ( image_bpp == 4 ) printf( "warning: RGBA\n" );
	assert( image_width == 4 && image_height % 4 == 0 && "bad dimensions..." );
	assert( image_bpp == 3 && "has alpha, and I'm lazy" );
	assert( image_data && "what the fuck" );
	
	frame_count = image_height / 4;

	//Reset the display
	dispcmd( DISP_BCL_ON );
	dispcmd( DISP_CBK );
	dispcmd( DISP_CLS );

	//Main animation loop

	while ( alive )
	{
		int i;
		for ( i = 0; i < frame_count; i++ )
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

	return 0;
}
