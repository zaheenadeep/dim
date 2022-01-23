#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <err.h>

#define TB_IMPL
#include "termbox.h"

enum {
      PEEKTIME = 1000, /* milliseconds */
      NLBUF    = 1024,
};
typedef struct Cursor Cursor;
typedef struct Line Line;
typedef struct Matrix Matrix;

struct Cursor {
	int x; /* starts at 0 */
	int y; /* starts at 0 */
};

struct Line {
	char    buf[NLBUF];   /* line buffer including \0 */
	int     nbuf;         /* buffer length excluding \0 */
};

struct Matrix {
	Line *lines;          /* holds every line in a file with */
	int  nlines;          /* number of lines */
};

struct tb_event  ev;
Cursor           cursor;
Matrix           matrix;
int              istart;

void
error(const char *s)
{
	fprintf(stderr, s);
}

void
shutdown(void)
{
	tb_shutdown();
}


char *
nfgets(char *buf, int size, FILE *stream)
{
	int ch;
	char *iter;

	iter = buf;	
	while (size > 1 && (ch = getc(stream)) != EOF
	       && ch != '\n' && ch != '\r') {
		*iter = ch;
		iter++;
		size--;
	}
	*iter = '\0';

	if (ch == EOF && iter == buf)
		/* If the only char scanned is EOF */
		return NULL;
	else
		return buf;
}

int
evget(void)
{
	int pe;
	
	pe = tb_peek_event(&ev, PEEKTIME);
	
	switch (pe) {
	case TB_OK:
	case TB_ERR_NO_EVENT:
		break;
	case TB_ERR_POLL:
		if (tb_last_errno() == EINTR)
			break;
		/* else fallthrough */
	default: /* all other ERRs */
		tb_strerror(tb_last_errno());
	}
	
	return pe;
}

void
setcursor(int x, int y)
{
	if (tb_set_cursor(x, y) < 0) {
		tb_strerror(tb_last_errno());
		return;
	}

	cursor.x = x;
	cursor.y = y;
}

void
evhandle(void)
{
	int ht, wd;
	Cursor *c;
	Matrix *m;
	
	switch (ev.type) {
	case TB_EVENT_KEY:
		break;
	case TB_EVENT_RESIZE:
		/* TODO */
		return;
	default:
		error("not an event key\n");
		return;
	}

	m = &matrix;
	c = &cursor;
	ht = tb_height();
	wd = tb_width();
	
	switch (ev.key) {
	case TB_KEY_ARROW_UP:
		if (c->y > 0)
			setcursor(c->x, c->y - 1);
		else if (istart >= 1)
			istart--;
		break;
	case TB_KEY_ARROW_DOWN:
		if (c->y <= ht - 2)
			setcursor(c->x, c->y + 1);
		else if (istart <= m->nlines - ht - 1)
			istart++;
		break;
	case TB_KEY_ARROW_LEFT:
		if (c->x > 0)
			setcursor(c->x - 1, c->y);
		break;
	case TB_KEY_ARROW_RIGHT:
		if (c->x < wd)
			setcursor(c->x + 1, c->y);
		break;
	case TB_KEY_HOME:
		setcursor(0, c->y);
		break;
	case TB_KEY_END:
		setcursor(m->lines[c->y].nbuf - 1, c->y);
		break;
	case TB_KEY_PGUP:
		istart -= ht;
		if (istart < 0)
			istart = 0;
		break;
	case TB_KEY_PGDN:
		istart += ht;
		if (istart + ht >= m->nlines)
			istart = m->nlines - ht;
		break;
	case TB_KEY_CTRL_Q:
		exit(0);
	}
}

void
matinit(void)
{
	Matrix *m;

	m = &matrix;
	m->lines = NULL;
	m->nlines = 0;
}

void
matfree(void)
{
	free(matrix.lines);
}

void
matloadfile(int argc, char *argv[])
{
	FILE *fp;
	Matrix *m;

	if (argc != 2) {
		error("usage: dim <filename>\n");
		exit(1);
	}

	fp = fopen(argv[1], "r");
	if (fp == NULL)
		err(1, "failed to open file\n");
	
	matinit();
	m = &matrix;
	for(;;) {
		char lbuf[NLBUF];
		char *fg;
		Line *larr, *lp;
		
		fg = nfgets(lbuf, NLBUF - 1, fp);
		if (fg == NULL) {
			fclose(fp);
			return;
		}

		/* allocate new Line in matrix */
		larr = reallocarray(m->lines, m->nlines + 1, sizeof(Line));
		if (larr == NULL) {
			matfree();
			fclose(fp);
			err(1, "failed to reallocate line buf\n");
		}
		m->lines = larr;
		m->nlines++;

		/* fill up new Line */
		lp = &(m->lines[m->nlines - 1]);
		strcpy(lp->buf, lbuf);
		lp->nbuf = strlen(lbuf) + 1;
	}
}

void
matdisplay(void)
{
	int c, r;
	Matrix *m;

	m = &matrix;
	for (r = 0; r < m->nlines && r < tb_height(); r++) {
		/*tb_print(0, r, TB_DEFAULT, TB_DEFAULT, m->lines[istart+r].buf);*/
		Line *lp;
		lp = &(m->lines[istart + r]);
		for (c = 0; c < lp->nbuf; c++)
			tb_set_cell(c, r, lp->buf[c], TB_DEFAULT, TB_DEFAULT);
	}
}

int
main(int argc, char *argv[])
{
	tb_init();
	atexit(shutdown);
	setcursor(0, 0);

      matloadfile(argc, argv);
	istart = 0;
	for(;;) {
		tb_clear();
		matdisplay();
		if (evget() < 0)
			goto end;
		evhandle();
	end:
		tb_present();
	}
	
	return 0;
}
