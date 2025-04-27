/*
MIT License

Copyright (c) 2025 ch-iv

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "raylib.h"

#define SCREEN_WIDTH 1000
#define SCREEN_HEIGHT 800
#define MAX_LINE_LEN 200
#define MAX_LINES 1024

Color BACKGROUND_COLOR = (Color){26, 27, 38, 255};
Color TYPE_COLOR = (Color){45, 188, 215, 255};
Color KEYWORD_COLOR = (Color){130, 95, 195, 255};
Color STRING_COLOR = (Color){162, 220, 80, 255};
Color DELIM_COLOR = (Color){170, 185, 255, 255};
Color TEXT_COLOR = (Color){170, 185, 255, 255};

float min_float(float a, float b) { return (a < b) ? a : b; }

char *types_list[] = {"double",   "int",      "long",    "char",     "float",
                      "short",    "unsigned", "signed",  "int8_t",   "uint8_t",
                      "int16_t",  "uint16_t", "int32_t", "uint32_t", "int64_t",
                      "uint64_t", "size_t",   "ssize_t", "off_t",    "int8_t"};

char *keywords_list[] = {
    "NULL",   "auto",   "struct",   "break",   "else",   "switch",
    "case",   "enum",   "register", "typedef", "extern", "return",
    "union",  "const",  "continue", "for",     "void",   "default",
    "goto",   "sizeof", "volatile", "do",      "if",     "static",
    "inline", "while"};

size_t types_count = sizeof(types_list) / sizeof(types_list[0]);
size_t keywords_count = sizeof(keywords_list) / sizeof(keywords_list[0]);

typedef struct Token {
  char *str;
  int size;
  Color color;
  bool delim;
} Token;

void colorize(Token *token) {
  if ((token->str)[0] == '"' || (token->str)[0] == '\'') {
    token->color = STRING_COLOR;
    return;
  }
  if (token->delim) {
    token->color = DELIM_COLOR;
    return;
  }
  for (size_t i = 0; i < types_count; i++) {
    if (strcmp(token->str, types_list[i]) == 0) {
      token->color = TYPE_COLOR;
      return;
    }
  }
  for (size_t i = 0; i < keywords_count; i++) {
    if (strcmp(token->str, keywords_list[i]) == 0) {
      token->color = KEYWORD_COLOR;
      return;
    }
  }

  token->color = TEXT_COLOR;
}

char chartype(char c) {
  if (c == '\'') {
    return 'c';
  }
  if (c == '"') {
    return 's';
  }
  char delims[] = " {}(),.[];";
  for (int i = 0; i < (int)strlen(delims); i++) {
    if (c == delims[i])
      return 'd';
  }
  return 't';
}

void tokenize(char *str, Token *tokens, int *ntokens) {
  *ntokens = 0;
  char type = '\0';
  bool skip_next = false;
  for (int i = 0; i < (int)strlen(str); i++) {
    char newtype = chartype(str[i]);
    if (type != newtype && type != 's' && type != 'c' && !skip_next) {
      if (*ntokens > 0) {
        char *copied_token =
            malloc(sizeof(char) * (tokens[(*ntokens) - 1].size + 1));
        strncpy(copied_token, tokens[(*ntokens) - 1].str,
                tokens[(*ntokens) - 1].size);
        copied_token[tokens[(*ntokens) - 1].size] = '\0';
        tokens[(*ntokens) - 1].str = copied_token;
      }
      (*ntokens)++;
      tokens[(*ntokens) - 1].str = &str[i];
      tokens[(*ntokens) - 1].size++;
      tokens[(*ntokens) - 1].delim = type == 't';
    } else {
      tokens[(*ntokens) - 1].size++;
    }
    if ((type == 's' && newtype == 's') || (type == 'c' && newtype == 'c')) {
      type = '\0';
      continue;
    }
    if ((type == 's' && newtype != 's') || (type == 'c' && newtype != 'c'))
      continue;
    type = newtype;
  }
  char *copied_token = malloc(sizeof(char) * (tokens[(*ntokens) - 1].size + 1));
  strncpy(copied_token, tokens[(*ntokens) - 1].str,
          tokens[(*ntokens) - 1].size);
  copied_token[tokens[(*ntokens) - 1].size] = '\0';
  tokens[(*ntokens) - 1].str = copied_token;
}

int main(int argc, char **argv) {
  if (argc != 2) {
    printf("Usage ./%s <filename.txt>\n", argv[0]);
    return 1;
  }

  FILE *f = fopen(argv[1], "r");
  if (f == NULL) {
    fprintf(stderr, "Can't open file %s\n", argv[1]);
    return 1;
  }

  char code[MAX_LINES][MAX_LINE_LEN];
  int lines = 0;

  for (int i = 0; i < MAX_LINES; i++) {
    if (fgets(code[i], MAX_LINE_LEN, f) == NULL) {
      break;
    }
    code[i][strlen(code[i]) - 1] = '\0';
    lines++;
  }

  Token tokens[MAX_LINES][MAX_LINE_LEN] = {0};
  int ntokens[MAX_LINES] = {0};

  for (int i = 0; i < lines; i++) {
    tokenize(code[i], tokens[i], &ntokens[i]);
  }

  for (int i = 0; i < lines; i++) {
    for (int j = 0; j < ntokens[i]; j++) {
      colorize(&tokens[i][j]);
    }
  }

  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Montage");
  SetWindowState(FLAG_WINDOW_RESIZABLE);
  SetTargetFPS(144);
  int fontSize = 30;
  int codx = 50;
  int cody = 10;

  Font myfont = LoadFontEx("./resources/fonts/iosevka/Iosevka-Medium.ttf",
                           fontSize, NULL, 0);
  SetTextureFilter(myfont.texture, TEXTURE_FILTER_POINT);
  while (!WindowShouldClose()) {
    BeginDrawing();
    ClearBackground(BACKGROUND_COLOR);
    cody += 35 * GetMouseWheelMove();
    int codcy = cody;
    int codcx = codx;
    int spacing = 0;
    for (int i = 0; i < lines; i++) {
      for (int j = 0; j < ntokens[i]; j++) {
        DrawTextEx(myfont, tokens[i][j].str, (Vector2){codcx, codcy}, fontSize,
                   spacing, tokens[i][j].color);
        codcx += MeasureTextEx(myfont, tokens[i][j].str, fontSize, spacing).x;
      }
      codcy += 35;
      codcx = codx;
    }

    EndDrawing();
  }
  for (int i = 0; i < lines; i++) {
    for (int j = 0; j < ntokens[i]; j++) {
      free(tokens[i][j].str);
    }
  }
  CloseWindow();
  return 0;
}
