/*** includes ***/

#include <asm-generic/errno-base.h>
#include <asm-generic/ioctls.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <string.h>

/*** defines ***/

#define KILO_VERSION "0.0.1"

#define CTRL_KEY(k) ((k) & 0x1f)

// defining ctrl q for quit

/*** data ***/

struct editorConfig {
	int screenrows;
	int screencols;
	struct termios orig_termios;
};	
struct editorConfig E;

struct abuf {
	char *b;
	int len;
//this is a append buffer, since c does not have dynamic strings we need this
};

#define ABUF_INIT {NULL, 0}
// this is a empty buffer


/*** prototype functions ***/

int getWindowSize(int *rows, int *cols);
void editorRefreshScreen();
void die(const char *s);
void disableRawMode();
void enableRawMode();
void editorProcessKeypress();
char editorReadKey();
int getCursorPosition(int *rows, int *cols);
void editorDrawRows(struct abuf *ab);

/*** init ***/

void initEditor(){
	if (getWindowSize(&E.screenrows, &E.screencols) == -1) die("getWindowSize");
}

int main(){
	
	enableRawMode();
	initEditor();

	while (1){
		editorRefreshScreen();
		editorProcessKeypress();
	}
	return 0;
}

/*** terminal ***/

void enableRawMode(){
	if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) die("tcgetattr");
	// reads terminal attributes into termios struc
	atexit(disableRawMode);
	// at exit we restore users terminal
	struct termios raw = E.orig_termios;
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
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1) die("tcgetattr");
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

int getCursorPosition(int *rows, int *cols){
	char buf[32];
	unsigned int i = 0;

	if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;

	while (i < sizeof(buf) - 1){ 
		if (read(STDIN_FILENO, &buf[i], 1 ) != 1) break; 
		if (buf[i] == 'R') break;
		i++;
	}
// use a buffer, we keep reading until R
	buf[i] = '\0';
	if (buf[0] != '\x1b' || buf[1] != '[') return -1;
	if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) return -1;
// make sure it responded with an escape sequence, we use sscanf to pass the tird char of buf, skipping \x1b and [.
// we also pass the string as %d;%d to parse to ints seperated by ; and put the values into rows and cols
	return 0;
}	

int getWindowSize(int *rows, int *cols){
	struct winsize ws;

	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws)== -1 || ws.ws_col == 0) {
		if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return -1;
		return getCursorPosition(rows, cols);
// second if is backup method, C is for moving cursor to right B moves it down 999 is the amount
	}
//if statement below works but we use a fallback method for finding screen size
//	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1|| ws.ws_col == 0) {
//		return -1;
//	}
// if there is and error with either column or row sizes return -1, struct winsize is from sys/ioctl we store our window size using that array
	else {
	*cols = ws.ws_col;
	*rows = ws.ws_row;
	return 0;
// on succcess we pass the values back by using pointers
	}
}	

/*** append buffer ***/

void abAppend(struct abuf *ab, const char *s, int len){
	char *newString = realloc(ab->b, ab->len + len);

	if (newString == NULL) return;
	memcpy(&newString[ab->len], s, len);
	ab->b = newString;
	ab->len += len;
// to append a string s to an abuf we first need the memory, realloc gives us a memory block the size of current string + the size of the string we are appending
// memcpy copies the current string s after the end of the current data in the buffer
}

void abFree(struct abuf *ab){
	free(ab->b);
// abfree is a destructor that deallocates the dynamic memory used by abuf
}	

/*** input ***/

void editorProcessKeypress(){
	char c = editorReadKey();

	switch (c) {
		case CTRL_KEY('q'):
			write(STDOUT_FILENO, "\x1b[2J", 4);
			write(STDOUT_FILENO, "\x1b[H", 3);
			exit(0);
			break;	
	}
// waits for keypress and handles it accordingly
}	

/*** output ***/

void editorDrawRows(struct abuf *ab) {
  int y;
  for (y = 0; y < E.screenrows; y++) {
    if (y == E.screenrows / 3) {
      char welcome[80];
      int welcomelen = snprintf(welcome, sizeof(welcome),
        "Kilo editor -- version %s", KILO_VERSION);
      if (welcomelen > E.screencols) welcomelen = E.screencols;
      int padding = (E.screencols - welcomelen) / 2;
      if (padding) {
        abAppend(ab, "~", 1);
        padding--;
      }
      while (padding--) abAppend(ab, " ", 1);
      abAppend(ab, welcome, welcomelen);
    } else {
      abAppend(ab, "~", 1);
    }
    abAppend(ab, "\x1b[K", 3);
    if (y < E.screenrows - 1) {
      abAppend(ab, "\r\n", 2);
    }
  }
}

void editorRefreshScreen(){
	struct abuf ab = ABUF_INIT;

	abAppend(&ab, "\x1b[?25l", 6);
// l and v below are for hiding the cursor while refreshing

	abAppend(&ab, "\x1b[H", 3);
// we expect 3 bytes H is for repositioning the cursor it normally takes 2 arquments which are row and collum but since both are 1 as default we don't need to change them we could have used 12;40H for row and column
	editorDrawRows(&ab);

	abAppend(&ab, "\x1b[H", 3);
	abAppend(&ab, "\x1b[?25h", 6);

	write(STDOUT_FILENO, ab.b, ab.len);
	abFree(&ab);
// after drawing ~'s we reposition the cursor
}	
