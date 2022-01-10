#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#define TB_IMPL
#include "termbox.h"

#define PEEKTIME 1000 /* milliseconds */

typedef struct Cursor Cursor;
struct Cursor {
	int x; /* starts at 0 */
	int y; /* starts at 0 */
};

struct tb_event ev;
Cursor cursor;

void
error(const char *s) {
	fprintf(stderr, s);
}

void
shutdown(void) {
	tb_shutdown();
}

int
evget(void) {
	int pe;
	
	pe = tb_peek_event(&ev, PEEKTIME);
	
	switch (pe) {
	case TB_OK:
	case TB_ERR_NO_EVENT:
		break;
	case TB_ERR_POLL:
		if (tb_last_errno() == EINTR)
			break;
		/* fallthrough */
	default: /* all other ERRs */
		tb_strerror(tb_last_errno());
	}
	
	return pe;
}

void
setcursor(int x, int y) {
	if (tb_set_cursor(x, y) < 0) {
		tb_strerror(tb_last_errno());
		return;
	}

	cursor.x = x;
	cursor.y = y;
}

void
evhandle(void) {
	int ht, wd;
	Cursor *c;
	
	if (ev.type != TB_EVENT_KEY) {
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

int
main()
{
	tb_init();
	atexit(shutdown);

	setcursor(0, 0);
	tb_present();
	
	for(;;) {
		if (evget() < 0)
			goto end;
		evhandle();
	end:
		tb_present();
	}
	
	return 0;
}
