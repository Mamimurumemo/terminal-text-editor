/*** includes ***/

#include <asm-generic/errno-base.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>

/*** defines ***/

#define CTRL_KEY(k) ((k) & 0x1f)
// defining ctrl q for quit

/*** data ***/

struct termios orig_termios;

/*** prototype functions ***/

void die(const char *s);
void disableRawMode();
void enableRawMode();

/*** init ***/

int main(){
	enableRawMode();

	while (1){
		
		char c = '\0';
		if (read(STDIN_FILENO, &c, 1) == -1 && errno != EAGAIN) die("read");
		if (iscntrl(c)) {
		// iscontrol checks if a pressed char is a control char(arrow keys etc.)
			printf("%d\r\n", c);
		}
		// \r\n is for fixing terminal \n
		else {
			printf("%d ('%c')\r\n", c, c);
		// %c is for writing out the bytes of a char
		}
		if (c == CTRL_KEY('q')) break;
		// pressing ctrl q exits program while loop only works until q is pressed
	}
	return 0;
}

/*** terminal ***/

void enableRawMode(){
	if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) die("tcgetattr");
	// reads terminal attributes into termios struc
	atexit(disableRawMode);
	// at exit we restore users terminal
	struct termios raw = orig_termios;
	// create a termips struct for operations
	raw.c_oflag &= ~(OPOST);
	// fixes \n for terminal
	raw.c_iflag &= ~(IXON | ICRNL | BRKINT | INPCK | ISTRIP);
	// icrnl turns fixes ctrl m
	// ıxon turns off ctrl s and ctrl q
	// everything else is misc.
	raw.c_cflag |= (CS8);
	// misc.
	raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
	// c_lfag is for local flags, we want to disable ECHO since it's responsible for displaying text as you type
	// ıcannon turns cannonical mode off
	// ısıg turns off terminal shortcuts like ctrl c and ctrl z
	// iexten turns off crtl v
	raw.c_cc[VMIN] = 0;
	// vmin is for min. number of bytes of input before read() can return, setting it to 0 makes it instant
	raw.c_cc[VTIME] = 1;
	// vtime sets the max amount of time wait before read() returns, setting it to 1 measn 100 miliseconds
	// if the user waits more than 100 miliseconds program returns 0
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcgetattr");
	// this applies them to the terminal

}
void disableRawMode(){
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1) die("tcgetattr");
	// disables raw mode and has error handling due to == -1
}

void die(const char *s){
	perror(s);
	exit(1);
	// standard error handling
}	
