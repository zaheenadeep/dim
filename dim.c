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
shutdown() {
	tb_shutdown();
}

int
main()
{
	tb_init();
	atexit(shutdown);

	tb_set_cursor(0, 0);
	tb_present();
	
	for(;;) {
		switch (tb_peek_event(&ev, PEEKTIME)) {
		case TB_OK:
			break;
		case TB_ERR_NO_EVENT:
			continue;
		case TB_ERR_POLL:
			if (tb_last_errno() == EINTR)
				continue;
			/* fallthrough */
		default: /* all other ERRs */
			tb_strerror(tb_last_errno());
			exit(1);
		}

		if (ev.type != TB_EVENT_KEY || ev.key == TB_KEY_CTRL_Q)
			exit(1);		
	}
	
	tb_present();
	
	return 0;
}
