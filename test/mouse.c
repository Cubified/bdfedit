/*
 * mouse.c: a minimal application to print raw
 * escape codes from mouse/keyboard input
 *
 * Requires tcc (for "C script")
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <termios.h>

void shutdown(int signo){
  printf("\x1b[?1003l\x1b[?1015l\x1b[?1006l");
  exit(0);
}

int main(){
  char buf[64];
  int nread;
  int x, y;
  char *tok;

  struct termios tio, raw;
  tcgetattr(STDIN_FILENO, &tio);
  raw = tio;
  raw.c_lflag &= ~(ECHO | ICANON);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

  printf("\x1b[?1003h\x1b[?1015h\x1b[?1006hPress Ctrl+C to exit.\n");
  signal(SIGINT, shutdown);

  while((nread=read(STDIN_FILENO, buf, sizeof(buf))) > 0){
    printf("%s\n", buf+1);
    continue;
    if(buf[0] == '\x1b' &&
       buf[1] == '[' &&
       buf[2] == '<'){
      switch(buf[3]){
        case '3':
          printf("move ");
          break;
        case '0':
          switch(buf[strlen(buf)-3]){
            case 'M':
              printf("down ");
              break;
            case 'm':
              printf("up ");
              break;
          }
          break;
      }
      tok = strtok(buf+3, ";");
      tok = strtok(NULL, ";");
      printf("(%s", tok);
      tok = strtok(NULL, ";");
      printf(", %i)\n", atoi(tok));
    }

    if(nread >= 5 &&
       buf[0] == '\x1b' &&
       buf[1] == '[' &&
       buf[2] == 'M'){
      switch(buf[3]){
        case ' ':
          printf("down ");
          break;
        case '#':
          printf("up ");
          break;
        case 'C':
        case '@':
          printf("move ");
          break;
      }
      x = (buf[4] & 0xff) - 0x20;
      y = (buf[5] & 0xff) - 0x20;
      if(x < 0) x += 0xff;
      printf("(%i, %i)\n", x, y);
    }

    if(buf[0] == 'q'){
      tcsetattr(STDIN_FILENO, TCSAFLUSH, &tio);
      exit(0);
    }
  }
 
  return 0;
}
