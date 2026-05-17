#include "hal.h"
#include <SDL2/SDL.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

// --- Conio Implementation ---
void clrscr(void) { printf("\033[H\033[J"); }

int getch(void) {
  struct termios oldt, newt;
  int ch;
  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  ch = getchar();
  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  return ch;
}

int kbhit(void) {
  struct termios oldt, newt;
  int ch;
  int oldf;
  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
  fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
  ch = getchar();
  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  fcntl(STDIN_FILENO, F_SETFL, oldf);
  if (ch != EOF) {
    ungetc(ch, stdin);
    return 1;
  }
  return 0;
}

// --- SDL Implementation ---
static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static SDL_Texture *texture = NULL;

int hal_init(const char *title, int width, int height) {
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
    return -1;
  }

  window =
      SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                       width * 2, height * 2, SDL_WINDOW_SHOWN);
  if (!window)
    return -1;

  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  if (!renderer)
    return -1;

  texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                              SDL_TEXTUREACCESS_STREAMING, width, height);
  if (!texture)
    return -1;

  return 0;
}

void hal_present(const uint32_t *buffer, int width, int height) {
  (void)height;
  SDL_UpdateTexture(texture, NULL, buffer, width * sizeof(uint32_t));
  SDL_RenderClear(renderer);
  SDL_RenderCopy(renderer, texture, NULL, NULL);
  SDL_RenderPresent(renderer);
}

int hal_handle_events(void) {
  SDL_Event e;
  while (SDL_PollEvent(&e)) {
    if (e.type == SDL_QUIT)
      return 0;
    if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)
      return 0;
  }
  return 1;
}

void hal_cleanup(void) {
  SDL_DestroyTexture(texture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
}

// --- Host I/O Implementation ---
void hal_log(const char *message) { printf("%s\n", message); }

void hal_exit(int code) { exit(code); }

int hal_load(const char *path, uint8_t *buffer, size_t size) {
  FILE *file = fopen(path, "rb");
  if (!file)
    return -1;
  size_t read = fread(buffer, 1, size, file);
  fclose(file);
  return (int)read;
}
