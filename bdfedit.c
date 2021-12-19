/*
 * bdfedit.c: a terminal-based font editor
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <signal.h>

#include "lang.h"
#include "ui.h"
#include "lib/bdf.h"

/*
 * PREPROCESSOR
 */
#define SCALE_FACTOR 3

#define HALFW (ws.ws_col / 2)
#define HALFH (ws.ws_row / 2)

#define GLYPHW (bdf->w)
#define GLYPHH (bdf->h)

#define GLYPHW_ADJ (GLYPHW * SCALE_FACTOR * 2)
#define GLYPHH_ADJ (GLYPHH * SCALE_FACTOR)

#define GLYPHSTARTX_CENTER (HALFW - (GLYPHW_ADJ / 2))
#define GLYPHSTARTY_CENTER (HALFH - (GLYPHH_ADJ / 2))

#define SCREENSPACE_TO_FONTSPACE_X(x, c) ((x - c) / (SCALE_FACTOR * 2))
#define SCREENSPACE_TO_FONTSPACE_Y(y, c) ((y - c) / SCALE_FACTOR)

#define FONTSPACE_TO_SCREENSPACE_X(x, c) ((x * SCALE_FACTOR * 2) + c)
#define FONTSPACE_TO_SCREENSPACE_Y(y, c) ((y * SCALE_FACTOR) + c)

#define GLYPHS_PER_ROW (ws.ws_col / ((GLYPHW * SCALE_FACTOR * 2) + (SCALE_FACTOR * 2)))
#define VIEWSTARTX_CENTER (1 + ((ws.ws_col - (GLYPHS_PER_ROW * ((GLYPHW * SCALE_FACTOR * 2) + SCALE_FACTOR))) / 2))

#define CENTER(len) (HALFW - (len / 2))

/*
 * GLOBALS
 */
enum modes {
  VIEW,
  EDIT,
  PREVIEW
};

struct termios tio;

char *filename;
char stdoutbuf[65535];

int mode = VIEW,
    viewmode_scroll = 0,
    prev_id,
    next_id,
    return_id;

bdf_font_t *bdf;
FILE *bdf_fp;

uint32_t current_glyph;

/*
 * STATIC DEFS
 */
static void buttons();
static void resize();
static void draw(int x, int y);
static void newmode(int m);
static void gen(box *b, char *out);
static void view_click(box *b, int x, int y);
static void edit_click(box *b, int x, int y);
static void edit_hover(box *b, int x, int y, int down);
static void text(box *b, char *out);
static void text_click(box *b, int x, int y);
static void text_hover(box *b);
static void init();
static void loop();
static void stop();
static void font();

/*
 * FUNCTIONS
 */
void buttons(){
  int x = 0, y = 0, i, f, g;
  bdf_char_t *c, *tmp;

  HASH_ITER(hh, bdf->table, c, tmp){
    for(f=0;f<GLYPHH;f++){
      for(g=0;g<GLYPHW;g++){
        ui_add(
          VIEWSTARTX_CENTER + (x * (GLYPHW_ADJ + SCALE_FACTOR)) + (g * SCALE_FACTOR * 2),
          y * (GLYPHH_ADJ + 2) + (f * SCALE_FACTOR),
          SCALE_FACTOR * 2,
          SCALE_FACTOR,
          VIEW,
          &(c->bmp[f]),
          c->bmp[f],
          gen,
          view_click,
          NULL,
          (f << 8) + g,
          c
        );
      }
    }

    x++;
    if(x == GLYPHS_PER_ROW){
      x = 0;
      y++;
    }
  }

  for(y=0;y<GLYPHH;y++){
    for(x=0;x<GLYPHW;x++){
      ui_add(
        GLYPHSTARTX_CENTER + (x * SCALE_FACTOR * 2),
        GLYPHSTARTY_CENTER + (y * SCALE_FACTOR),
        SCALE_FACTOR * 2,
        SCALE_FACTOR,
        EDIT,
        &current_glyph,
        0,
        gen,
        edit_click,
        edit_hover,
        (y << 8) + x,
        NULL
      );
    }
  }

  ui_add(
    CENTER(LANG_NOW_EDITING_LEN),
    GLYPHSTARTY_CENTER - 2,
    LANG_NOW_EDITING_LEN,
    1,
    EDIT,
    &current_glyph,
    0,
    text,
    text_click,
    text_hover,
    LANG_NOW_EDITING,
    NULL
  );
  next_id = ui_add(
    GLYPHSTARTX_CENTER + GLYPHW_ADJ + 2,
    HALFH,
    LANG_NEXT_GLYPH_LEN,
    1,
    EDIT,
    &current_glyph,
    0,
    text,
    text_click,
    text_hover,
    LANG_NEXT_GLYPH,
    NULL
  );
  prev_id = ui_add(
    GLYPHSTARTX_CENTER - LANG_PREV_GLYPH_LEN - 2,
    HALFH,
    LANG_PREV_GLYPH_LEN,
    1,
    EDIT,
    &current_glyph,
    0,
    text,
    text_click,
    text_hover,
    LANG_PREV_GLYPH,
    NULL
  );
  return_id = ui_add(
    CENTER(LANG_RETURN_LEN),
    GLYPHSTARTY_CENTER + GLYPHH_ADJ + 2,
    LANG_RETURN_LEN,
    1,
    EDIT,
    &current_glyph,
    0,
    text,
    text_click,
    text_hover,
    LANG_RETURN,
    NULL
  );
}

void resize(){
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
  ui_clear();
  buttons();
  ui_draw();
}

void draw(int x, int y){
  bdf_char_t *c;
  HASH_FIND_INT(bdf->table, &current_glyph, c);
  if(c == NULL) return;
  c->bmp[bdf->size-y] ^= 1 << (8-x);
}

void newmode(int m){
  mode = m;
  if(mode == VIEW) canscroll = 1;
  else canscroll = 0;
  printf("\x1b[0m\x1b[2J");
  ui_screen(mode);
  ui_draw();
}

/* TODO: More operations than necessary */
void gen(box *b, char *out){
  int x = ((int)b->data) & 0xff,
      y = ((int)b->data) >> 8,
      xt, yt;
  bdf_char_t *c;

  if(mode == EDIT){
    HASH_FIND_INT(bdf->table, &current_glyph, c);
  } else c = (bdf_char_t*)(b->data2);
  if(c == NULL) return;

  if(x != 0 && c->bmp[bdf->size-y] & (1 << (8-x))) sprintf(out, "\x1b[47m");
  else sprintf(out, "\x1b[48;2;0;0;0m");

  for(yt=0;yt<SCALE_FACTOR;yt++){
    for(xt=0;xt<SCALE_FACTOR*2;xt++){
      strcat(out, " ");
    }
    strcat(out, "\n");
  }
}

void view_click(box *b, int x, int y){
  current_glyph = ((bdf_char_t*)(b->data2))->codepoint;
  newmode(EDIT);
}

void edit_click(box *b, int x, int y){
  draw(
    SCREENSPACE_TO_FONTSPACE_X(x, GLYPHSTARTX_CENTER),
    SCREENSPACE_TO_FONTSPACE_Y(y, GLYPHSTARTY_CENTER)
  );
  force = 1;
  ui_draw_one(b, 1);
  force = 0;
}
void edit_hover(box *b, int x, int y, int down){
}

void text(box *b, char *out){
  bdf_char_t *c;
  uint32_t tmp = current_glyph;

  if(b->id == return_id){
    sprintf(out, b->data);
    return;
  }

  if(b->id == next_id) tmp += 1;
  else if(b->id == prev_id) tmp -= 1;

  HASH_FIND_INT(bdf->table, &tmp, c);
  if(c == NULL) return;

  sprintf(out, b->data, c->codepoint);
}
void text_click(box *b, int x, int y){
  if(b->id == next_id){
    current_glyph++;
    newmode(EDIT);
  } else if(b->id == prev_id){
    current_glyph--;
    newmode(EDIT);
  } else if(b->id == return_id){
    newmode(VIEW);
  }
}
void text_hover(box *b){
  if(b->id == next_id || b->id == prev_id){
    // TODO
  }
}

void init(){
  struct termios raw;

  signal(SIGTERM,  stop);
  signal(SIGQUIT,  stop);
  signal(SIGINT,   stop);
  signal(SIGWINCH, resize);

  setvbuf(stdout, stdoutbuf, _IOFBF, sizeof(stdoutbuf));

  ui_new(VIEW);
  resize();

  tcgetattr(STDIN_FILENO, &tio);
  raw = tio;
  raw.c_lflag &= ~(ECHO | ICANON);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void loop(){
  char buf[256];
  int nread, x, y, xc, yc, mouse = 0;

  ui_draw();

  while((nread=read(STDIN_FILENO, buf, sizeof(buf))) > 0){
    ui_update(buf, nread);

    if(nread == 1 &&
       (buf[0] == '\x1b' || buf[0] == 'q')){
      if(mode == VIEW) stop();
      else newmode(VIEW);
    }
  }
}

void stop(){
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &tio);
  ui_free();
  if((bdf_fp=fopen(filename, "w")) != NULL){
    bdf_write(bdf, bdf_fp);
  }
  exit(0);
}

void font(){
  int out;

  bdf = malloc(sizeof(bdf_font_t));
  if((bdf_fp=fopen(filename, "r")) != NULL){
    out = bdf_parse(bdf, bdf_fp);
    if(out == bdf_err_file) printf("Error: Failed to open file \"%s\".\n", filename);
    if(out == bdf_err_font) printf("Error: File \"%s\" is an invalid BDF file.\n", filename);
    if(out == bdf_err_read) printf("Error: File \"%s\" ended prematurely.\n", filename);

    if(out != 0) exit(out);
  }
}

/*
 * MAIN
 */
int main(int argc, char **argv){
  if(argc < 2){
    printf("Usage: bdfedit [font.bdf]\n");
    exit(0);
  }
  filename = argv[1];

  font();
  init();
  loop();
  stop();

  return 0;
}
