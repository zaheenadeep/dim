#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <err.h>

#define TB_IMPL
#include "termbox.h"

/* TODO: see what MLE does to not overwhelm CPU */
enum
{
	PEEKTIME	= 100,	/* milliseconds */
	NLBUF		= 1024,
};

typedef struct Cursor Cursor;
typedef struct Line Line;
typedef struct Matrix Matrix;

struct Cursor
{
	int	x;		/* starts at 0 */
	int	y;		/* starts at 0 */
};

struct Line
{
	char	buf[NLBUF];	/* line buffer including \0 */
	int	nbuf;		/* buffer length excluding \0 */
};

struct Matrix
{
	Line	*lines;		/* holds every line in a file with */
	int	nlines;		/* number of lines */
};

struct tb_event	ev;		/* current event */

Cursor	cursor;		/* current cursor position */
Matrix	mat;		/* the entire display */
int	irstart;	/* row starting index for display */
int	icstart;	/* col starting index for display */

void
shut(int stat)
{
	tb_shutdown();
	exit(stat);
}

void
eshut(int stat, const char *s)
{
	tb_shutdown();
	fputs(s, stderr);
	exit(stat);
}

/*TODO print error in the bottom row*/
void
error(const char *s)
{
	fputs(s, stderr);
	fflush(stderr);
}

char *
nfgets(char *buf, int size, FILE *stream)
{
	register int ch;
	register char *iter;

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
maxnline(void)
{
	int i, n;
	
	n = 0;
	for (i = 0; i < mat.nlines; i++)
		if (mat.lines[i].nbuf > n)
			n = mat.lines[i].nbuf;
	return n;
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
		error("not an event key\n");
		return;
	}

	c = &cursor;
	ht = tb_height();
	wd = tb_width();
	
	switch (ev.key) {
	case TB_KEY_ARROW_UP:
		if (c->y > 0)
			setcursor(c->x, c->y - 1);
		else if (irstart >= 1)
			irstart--;
		break;
	case TB_KEY_ARROW_DOWN:
		if (c->y <= ht - 2)
			setcursor(c->x, c->y + 1);
		else if (irstart <= mat.nlines - ht - 1)
			irstart++;
		break;
	case TB_KEY_ARROW_LEFT:
		if (c->x > 0)
			setcursor(c->x - 1, c->y);
		else if (icstart >= 1)
			icstart--;
		break;
	case TB_KEY_ARROW_RIGHT:
		if (c->x <= wd - 2)
			setcursor(c->x + 1, c->y);
		else if (icstart <= maxnline() - wd - 1)
			icstart++;
		break;
	case TB_KEY_HOME:
		setcursor(0, c->y);
		break;
	case TB_KEY_END:
		setcursor(mat.lines[c->y].nbuf - 1, c->y);
		break;
	case TB_KEY_PGUP:
		irstart -= ht;
		if (irstart < 0)
			irstart = 0;
		break;
	case TB_KEY_PGDN:
		irstart += ht;
		if (irstart + ht >= mat.nlines)
			irstart = mat.nlines - ht;
		break;
	case TB_KEY_CTRL_Q:
		shut(0);
	}
}

void
matinit(void)
{
	mat.lines = NULL;
	mat.nlines = 0;
}

void
matfree(void)
{
	free(mat.lines);
}

void
matloadfile(int argc, char *argv[])
{
	FILE *fp;

	if (argc != 2)
		eshut(1, "usage: dim <filename>\n");

	fp = fopen(argv[1], "r");
	if (fp == NULL)
		eshut(1, strerror(errno));
	
	matinit();
	for (;;) {
		char lbuf[NLBUF];
		char *fg;
		Line *larr, *lp;
		
		fg = nfgets(lbuf, NLBUF - 1, fp);
		if (fg == NULL) {
			fclose(fp);
			return;
		}

		/* allocate new Line in mat */
		larr = reallocarray(mat.lines, mat.nlines + 1, sizeof(Line));
		if (larr == NULL) {
			matfree();
			fclose(fp);
			eshut(1, strerror(errno));
		}
		mat.lines = larr;
		mat.nlines++;

		/* fill up new Line */
		lp = &(mat.lines[mat.nlines - 1]);
		strcpy(lp->buf, lbuf);
		lp->nbuf = strlen(lbuf) + 1;
	}
}

void
matdisplay(void)
{
	int c, r;

	for (r = 0; r < mat.nlines && r < tb_height(); r++) {
		/*tb_print(0, r, TB_DEFAULT, TB_DEFAULT, mat.lines[irstart+r].buf);*/
		Line *lp;
		lp = &(mat.lines[irstart + r]);
		for (c = 0; c < lp->nbuf; c++)
			tb_set_cell(c, r, lp->buf[icstart + c], TB_DEFAULT, TB_DEFAULT);
	}
}

int
main(int argc, char *argv[])
{
	tb_init();
	setcursor(0, 0);

	matloadfile(argc, argv);
	irstart = icstart = 0;
	for (;;) {
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
