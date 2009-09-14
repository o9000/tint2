#ifndef TOOLTIP_H
#define TOOLTIP_H

#include <sys/time.h>

#include "task.h"

enum tooltip_state {
	TOOLTIP_ABOUT_TO_SHOW,
	TOOLTIP_ABOUT_TO_HIDE,
};

typedef struct {
	Task* task;
	Window window;
	struct itimerval show_timeout;
	struct itimerval hide_timeout;
	Bool enabled;
	enum tooltip_state current_state;
	Bool mapped;
	int paddingx;
	int paddingy;
	PangoFontDescription* font_desc;
	config_color font_color;
	Color background_color;
	Border border;
} Tooltip;

extern Tooltip g_tooltip;


void init_tooltip();
void tooltip_sighandler(int sig);
void tooltip_trigger_show(Task* task, int x, int y);
void tooltip_show();
void tooltip_update();
void tooltip_trigger_hide();
void tooltip_hide();

#endif // TOOLTIP_H
