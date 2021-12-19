/*
 * bdf.h: a .bdf parser
 */

#ifndef __BDF_H
#define __BDF_H

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "uthash.h"

#define BDF_LINE 256
#define BDF_CHARS 1033
#define BDF_BITMAP 16

#define LINE_IS(decl) (strncmp(lbuf, decl, strlen(decl)) == 0)
#define GET_ARG() sscanf(lbuf, "%s %i", throwaway, &arg)

enum bdf_errors {
  bdf_err_file = 1,
  bdf_err_font = 2,
  bdf_err_read = 3
};

enum bdf_states {
  bdf_state_none,
  bdf_state_font,
  bdf_state_char,
  bdf_state_prop,
  bdf_state_btmp
};

typedef struct bdf_prop_t {
  char *name;
  char *str_val;
  int int_val;
  UT_hash_handle hh;
} bdf_prop_t;

typedef struct bdf_char_t {
  uint32_t codepoint;
  char *name;
  char x;
  char y;
  char w;
  char h;
  char bmp[(16*16)/8];
  UT_hash_handle hh;
} bdf_char_t;

typedef struct bdf_font_t {
  char *name;
  char size;
  char x;
  char y;
  char w;
  char h;
  int nchars;
  bdf_prop_t *props;
  bdf_char_t *table;
} bdf_font_t;

int bdf_parse(bdf_font_t *f, FILE *fp);
int bdf_write(bdf_font_t *f, FILE *fp);

#endif
