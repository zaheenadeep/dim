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
	char    buf[NLBUF];   /* line buffer including \n \r and \0 */
	int     nbuf;         /* buffer length excluding \0 */
};

struct Matrix {
	Line *lines;          /* holds every line in a file with */
	int  nlines;          /* number of lines */
};

struct tb_event ev;
Cursor cursor;
Matrix matrix;

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
	
	switch (ev.type) {
	case TB_EVENT_KEY:
		break;
	case TB_EVENT_RESIZE:
		/* TODO */
		return;
	default:
		error("not an event key");
		return;
	}

	c = &cursor;
	ht = tb_height();
	wd = tb_width();
	
	switch (ev.key) {
	case TB_KEY_ARROW_UP:
		if (c->y > 0)
			setcursor(c->x, c->y - 1);
		break;
	case TB_KEY_ARROW_DOWN:
		if (c->y < ht)
			setcursor(c->x, c->y + 1);
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
		setcursor(wd - 1, c->y);
		break;
	case TB_KEY_PGUP:
		setcursor(c->x, 0);
		break;
	case TB_KEY_PGDN:
		setcursor(c->x, ht - 1);
		break;
	case TB_KEY_CTRL_Q:
		exit(0);
	}
}

void
matloadfile(int argc, char *argv[])
{
	FILE *fp;
	char *fg;
	Matrix *mp;

	if (argc != 2) {
		error("usage: dim <filename>\n");
		exit(1);
	}

	fp = fopen(argv[1], "r");
	if (fp == NULL)
		err(1, "failed to open file");

	mp = &matrix;
	mp->lines = NULL;
	mp->nlines = 0;
	for(;;) {
		Line *larr;
		char lbuf[NLBUF];
		Line *newlp;
		
		fg = fgets(lbuf, NLBUF - 1, fp);
		if (fg == NULL) {
			fclose(fp);
			return;
		}

		larr = reallocarray(mp->lines, mp->nlines + 1, sizeof(Line));
		if (larr == NULL) {
			free(mp->lines);
			fclose(fp);
			err(1, "failed to reallocate line buf");
		}
		mp->lines = larr;
		mp->nlines++;
	        
		newlp = &(mp->lines[mp->nlines - 1]);
		strcpy(newlp->buf, lbuf);
		newlp->nbuf = strlen(lbuf) + 1;
	}
}

void matdisplay()
{
	
}

int
main(int argc, char *argv[])
{
	tb_init();
	atexit(shutdown);

	setcursor(0, 0);
	tb_present();

	matloadfile(argc, argv);

        for(;;) {
		if (evget() < 0)
			goto end;
		evhandle();
	end:
		tb_present();
	}
	
	return 0;
}
