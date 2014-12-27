#ifndef AURIOL_DB_H_
#define AURIOL_DB_H_

void signal_callback_handler( int signum );

unsigned char hex2char( unsigned char c );
unsigned char reverse_8bits( unsigned char n );
void osv2_parse( char *s );
void vent_parse( char *s );
void parse_input( char *s );

void *uart_receive( void *ptr );

int main(int argc, char *argv[]);

#endif
