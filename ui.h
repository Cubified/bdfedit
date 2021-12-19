/*
 * ui.h: simple tui library
 */

#pragma once

#include <stdlib.h>
#include <string.h>
#include "lib/vec.h"

/*
 * PREPROCESSOR
 */
#define ui_screen(s) screen = s;force = 1

#define box_contains(x, y, b) (x >= b->x && x <= b->x + b->w && y >= b->y && y <= b->y + b->h)

#define CURSOR_Y(b) (b->y+(n+1)+(canscroll ? scroll : 0))

#define CACHESIZE 8192

/*
 * TYPES
 */
typedef void (*func)();
typedef struct box {
  int id;

  int x, y;
  int w, h;

  int screen;

  char *cache;
  char *watch;
  char last;

  func draw;
  func onclick;
  func onhover;

  void *data;
  void *data2;
} box;
typedef vec_t(box*) vec_box_t;

/*
 * GLOBALS
 */
struct winsize ws;
vec_box_t ui;
int mouse;
int screen, scroll, canscroll;
int id;
int force;

/*
 * UI
 */
void ui_new(int screen){
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);

  vec_init(&ui);
  
  printf("\x1b[?1049h\x1b[0m\x1b[2J\x1b[?1003h\x1b[?1015h\x1b[?1006h\x1b[?25l");

  mouse = 0;

  screen = screen;
  scroll = 0;
  canscroll = 1;
  
  id = 0;

  force = 0;
}

void ui_free(){
  box *val;
  int i;
  char *term;

  printf("\x1b[0m\x1b[2J\x1b[?1049l\x1b[?1003l\x1b[?1015l\x1b[?1006l\x1b[?25h");

  vec_foreach(&ui, val, i){
    free(val);
  }
  vec_deinit(&ui);

  term = getenv("TERM");
  if(strncmp(term, "screen", 6) == 0 ||
     strncmp(term, "tmux", 4) == 0){
    printf("Note: Terminal multiplexer detected.\n  For best performance (i.e. reduced flickering), running natively inside\n  a GPU-accelerated terminal such as alacritty or kitty is recommended.\n");
  }
}

int ui_add(
  int x, int y, int w, int h, int screen,
  char *watch, char initial,
  func draw, func onclick, func onhover,
  void *data, void *data2
){
  box *b = malloc(sizeof(box));

  b->id = id++;

  b->x = x;
  b->y = y;
  b->w = w;
  b->h = h;

  b->screen = screen;

  b->cache = malloc(CACHESIZE);
  b->watch = watch;
  b->last = initial;

  b->draw = draw;
  b->onclick = onclick;
  b->onhover = onhover;

  b->data = data;
  b->data2 = data2;

  vec_push(&ui, b);
  draw(b, b->cache);

  return b->id;
}

void ui_clear(){
  // TODO: This is very very leaky
  vec_clear(&ui);
  printf("\x1b[0m\x1b[2J");
}

void ui_draw_one(box *tmp, int flush){
  char buf[CACHESIZE], *tok;
  int n = -1;

  if(tmp->screen != screen) return;
  
  memset(buf, 0, sizeof(buf));
  if(force || (tmp->watch != NULL && *(tmp->watch) != tmp->last)){
    tmp->draw(tmp, buf);
    if(tmp->watch != NULL) tmp->last = *(tmp->watch);
    strcpy(tmp->cache, buf);
  } else {
    /* Both strings are guaranteed to have the same max length, strcpy is safe */
    strcpy(buf, tmp->cache);
  }
  tok = strtok(buf, "\n");
  while(tok != NULL){
    if(tmp->x > 0 &&
       tmp->x < ws.ws_col &&
       CURSOR_Y(tmp) > 0 &&
       CURSOR_Y(tmp) < ws.ws_row){
      printf("\x1b[%i;%iH%s", CURSOR_Y(tmp), tmp->x, tok);
      n++;
    }
    tok = strtok(NULL, "\n");
  }

  if(flush) fflush(stdout);
}

void ui_draw(){
  box *tmp;
  int i;

  printf("\x1b[0m\x1b[2J");

  vec_foreach(&ui, tmp, i){
    ui_draw_one(tmp, 0);
  }
  fflush(stdout);
  force = 0;
}

void ui_update(char *c, int n){
  box *tmp;
  int ind, x, y;
  char cpy[n], *tok;

  if(n >= 4 &&
     c[0] == '\x1b' &&
     c[1] == '[' &&
     c[2] == '<'){
    strncpy(cpy, c, n);
    tok = strtok(cpy+3, ";");
    
    switch(tok[0]){
      case '0':
        mouse = (strchr(c, 'm') != NULL);
        if(mouse){
          tok = strtok(NULL, ";");
          x = atoi(tok);
          tok = strtok(NULL, ";");
          y = strtol(tok, NULL, 10) - (canscroll ? scroll : 0);

          vec_foreach(&ui, tmp, ind){
            if(tmp->screen == screen &&
               box_contains(x, y, tmp) &&
               tmp->onclick != NULL){
              tmp->onclick(tmp, x, y);
            }
          }
        }
        break;
      case '3':
        /* TODO: Is this even necessary? It's a lot of overhead */
        break;
      case '6':
        if(canscroll){
          scroll += (tok[1] == '4' ? 2 : -2);
          printf("\x1b[0m\x1b[2J");
          ui_draw();
        }
        break;
    }
  }
}
