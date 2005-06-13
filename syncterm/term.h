#ifndef _TERM_H_
#define _TERM_H_

#include "bbslist.h"

struct terminal {
	int	height;
	int	width;
	int	x;
	int	y;
	int nostatus;
};

extern struct terminal term;
extern int backlines;

BOOL doterm(struct bbslist *);
void mousedrag(unsigned char *scrollback);

#endif
