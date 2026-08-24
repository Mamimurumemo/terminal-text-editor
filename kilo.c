#include <termios.h>
#include <unistd.h>
#include <stdlib.h>

struct termios orig_termios;

void disableRawMode(){
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}	

void enableRawMode(){
	tcgetattr(STDIN_FILENO, &orig_termios);
	// reads terminal attributes into termios struc
	atexit(disableRawMode);
	// at exit we restore users terminal
	struct termios raw = orig_termios;
	// create a termips struct for operations
	raw.c_lflag &= ~(ECHO | ICANON);
	// c_lfag is for local flags, we want to disable ECHO since it's responsible for displaying text as you type
	// ıcannon turns cannonical mode off
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
	// this applies them to the terminal

}	

int main(){
	enableRawMode();

	char c;
	while (read(STDIN_FILENO, &c, 1) == 1 && c != 'q');
	// pressing q exits program while loop only works until q is pressed

	return 0;
}

