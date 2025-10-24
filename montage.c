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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "raylib.h"

#define begin_timer()                                                          \
  do {                                                                         \
    _timer_start = clock();                                                    \
  } while (0)

#define end_timer()                                                            \
  do {                                                                         \
    clock_t _timer_end = clock();                                              \
    double elapsed_time =                                                      \
        ((double)(_timer_end - _timer_start)) / CLOCKS_PER_SEC;                \
    printf("Elapsed time: %f seconds\n", elapsed_time);                        \
  } while (0)

#define SCREEN_WIDTH 1000
#define SCREEN_HEIGHT 800
#define MAX_LINE_LEN 200
#define MAX_LINES 1024

clock_t _timer_start;

Color BACKGROUND_COLOR = (Color){26, 27, 38, 255};
Color TYPE_COLOR = (Color){45, 188, 215, 255};
Color KEYWORD_COLOR = (Color){130, 95, 195, 255};
Color STRING_COLOR = (Color){162, 220, 80, 255};
Color DELIM_COLOR = (Color){170, 185, 255, 255};
Color TEXT_COLOR = (Color){170, 185, 255, 255};
Color TEXT_COLOR_DARKER = (Color){58, 65, 95, 255};

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

bool containsPointRect(Vector2 point, Rectangle rect) {
  return (point.x >= rect.x) && (point.x <= (rect.x + rect.width)) &&
         (point.y >= rect.y) && (point.y <= (rect.y + rect.height));
}

int alignRight(Rectangle child, Rectangle parent) {
  return parent.width - child.width;
}

typedef struct Token {
  char *str;
  int size;
  Color color;
  bool delim;
} Token;

typedef struct Code {
  char (*lines)[MAX_LINE_LEN];
  Token (*tokens)[MAX_LINE_LEN];
  int *ntokens;
  int nlines;
} Code;

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

void getCodeTexture(RenderTexture2D *target, int textureWidth, Font font,
                    int fontSize, int charSpacing, int lineHeight, Code *code,
                    bool drawLineNumbers) {
  int textureHeight = code->nlines * lineHeight;
  *target = LoadRenderTexture(textureWidth, textureHeight);
  BeginTextureMode(*target);
  int digitWidth = MeasureTextEx(font, "0", fontSize, charSpacing).x;
  int maxLineDigits = (int)(log10(code->nlines)) + 1;
  int lineNumOffset = digitWidth * maxLineDigits + 15;
  char snum[maxLineDigits + 1];
  int currTextX = drawLineNumbers ? lineNumOffset : 0;
  int currTextY = 0;
  for (int i = 0; i < code->nlines; i++) {
    if (drawLineNumbers) {
      int lineDigits = (int)(log10(i)) + 1;
      int lineNumX =
          alignRight((Rectangle){0, 0, lineDigits * digitWidth, 0},
                     (Rectangle){0, 0, maxLineDigits * digitWidth, 0});
      sprintf(snum, "%d", i);
      DrawTextEx(font, snum, (Vector2){lineNumX, currTextY}, fontSize,
                 charSpacing, TEXT_COLOR_DARKER);
    }

    for (int j = 0; j < code->ntokens[i]; j++) {
      DrawTextEx(font, code->tokens[i][j].str, (Vector2){currTextX, currTextY},
                 fontSize, charSpacing, code->tokens[i][j].color);
      currTextX +=
          MeasureTextEx(font, code->tokens[i][j].str, fontSize, charSpacing).x;
    }

    currTextY += 35;
    currTextX = drawLineNumbers ? lineNumOffset : 0;
  }
  EndTextureMode();
}

Code parseCode(char *filename) {
  FILE *f = fopen(filename, "r");
  if (f == NULL) {
    fprintf(stderr, "Can't open file %s\n", filename);
    exit(EXIT_FAILURE);
  }

  Code code;
  code.lines = calloc(MAX_LINES, sizeof(char[MAX_LINE_LEN]));
  code.tokens = calloc(MAX_LINES, sizeof(Token[MAX_LINE_LEN]));
  code.ntokens = calloc(MAX_LINES, sizeof(int));
  code.nlines = 0;
  for (int i = 0; i < MAX_LINES; i++) {
    if (fgets(code.lines[i], MAX_LINE_LEN, f) == NULL) {
      break;
    }
    code.lines[i][strlen(code.lines[i]) - 1] = '\0';
    code.nlines++;
  }

  for (int i = 0; i < code.nlines; i++) {
    tokenize(code.lines[i], code.tokens[i], &code.ntokens[i]);
  }

  for (int i = 0; i < code.nlines; i++) {
    for (int j = 0; j < code.ntokens[i]; j++) {
      colorize(&(code.tokens[i][j]));
    }
  }
  return code;
}

double getSmoothScroll(double startTime, double startPos, double currTime,
                       double timeToMove, double distToMove) {
  double ratio = (currTime - startTime) / timeToMove;
  if (ratio > 1) {
    ratio = 1.0;
  }
  ratio = 1 - (1 - ratio) * (1 - ratio);
  return startPos + ratio * distToMove;
}

typedef struct Container {
  double codeY;
  double scrollStartTime;
  double scrollStartPos;
  double scrollDirection;
  bool scrolling;
  RenderTexture2D target;
  Rectangle rect;
  Rectangle view;
} Container;

void drawCodeInContainer(Container *c) {
  if (GetMouseWheelMove() && containsPointRect(GetMousePosition(), c->rect)) {
    c->scrollStartTime = GetTime();
    c->scrollStartPos = c->codeY;
    c->scrollDirection = -GetMouseWheelMove();
    c->scrolling = true;
  }

  if (c->scrolling) {
    double newY = getSmoothScroll(c->scrollStartTime, c->scrollStartPos,
                                  GetTime(), 0.15, 200 * c->scrollDirection);
    if (newY < 0)
      newY = 0;
    if (newY + c->rect.height > c->target.texture.height)
      newY = c->target.texture.height - c->rect.height;
    if (newY == c->codeY) {
      c->scrolling = false;
    } else {
      c->codeY = newY;
    }
  }

  BeginBlendMode(3);

  SetTextureWrap(c->target.texture, TEXTURE_WRAP_CLAMP);
  DrawTextureRec(
      c->target.texture,
      (Rectangle){0, c->target.texture.height - c->codeY - c->rect.height,
                  c->rect.width, -c->rect.height},
      (Vector2){c->rect.x, c->rect.y}, RAYWHITE);
  EndBlendMode();
}

int main(int argc, char **argv) {
  if (argc != 2) {
    printf("Usage ./%s <filename.txt>\n", argv[0]);
    return 1;
  }

  Code code = parseCode(argv[1]);

  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Montage");
  SetWindowState(FLAG_WINDOW_RESIZABLE);
  MaximizeWindow();

  int fontSize = 30;
  int lineHeight = fontSize + 5;
  int charSpacing = 0;

  Font myfont = LoadFontEx("./resources/fonts/iosevka/Iosevka-Medium.ttf",
                           fontSize, NULL, 0);
  Container c1 = {0};

  while (!WindowShouldClose()) {
    if (c1.target.texture.width != GetScreenWidth() ||
        c1.rect.height != GetScreenHeight()) {

      c1.rect = (Rectangle){0, 0, GetScreenWidth(), GetScreenHeight()};
      getCodeTexture(&c1.target, GetScreenWidth(), myfont, fontSize,
                     charSpacing, lineHeight, &code, true);
    }
    BeginDrawing();
    ClearBackground(BACKGROUND_COLOR);
    drawCodeInContainer(&c1);
    EndDrawing();
  }

  CloseWindow();

  for (int i = 0; i < code.nlines; i++) {
    for (int j = 0; j < code.ntokens[i]; j++) {
      free(code.tokens[i][j].str);
    }
  }

  free(code.lines);
  free(code.tokens);
  free(code.ntokens);

  return 0;
}
