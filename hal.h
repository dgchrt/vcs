#ifndef CONIO_H
#define CONIO_H

void clrscr(void);
int kbhit(void);
int getch(void);

#define TERM_CLEAR_SCREEN "\033[H\033[J"

#endif /* CONIO_H */
