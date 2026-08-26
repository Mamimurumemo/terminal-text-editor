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

void editorRefreshScreen();
void die(const char *s);
void disableRawMode();
void enableRawMode();
void editorProcessKeypress();
char editorReadKey();

/*** init ***/

int main(){
	
	editorRefreshScreen();
	enableRawMode();

	while (1){
		editorProcessKeypress();
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

char editorReadKey(){
	int nread;
	char c;
	while ((nread = read(STDIN_FILENO, &c, 1)) != 1){
		if (nread == -1 && errno != EAGAIN) die("read");
	}
	return c;
// this func just waits for one keypress and returns it
}	

/*** input ***/

void editorProcessKeypress(){
	char c = editorReadKey();

	switch (c) {
		case CTRL_KEY('q'):
			exit(0);
			break;	
	}
// waits for keypress and handles it accordingly
}	

/*** output ***/

void editorRefreshScreen(){
	write(STDOUT_FILENO, "\x1b[2j", 4);
// 4 means we are expecting 4 bytes, \x1b is the escape char [ is required for it j is for clearing the screen and 2 is it's arqument which says the entire screen
	write(STDOUT_FILENO, "\x1b[H", 3);
// we expect 3 bytes H is for repositioning the cursor it normally takes 2 arquments which are row and collum but since both are 1 as default we don't need to change them we could have used 12;40H for row and column
}	
