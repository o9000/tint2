#include "net.h"
#include "docker.h"
#include "icons.h"
#include <assert.h>

Atom net_opcode_atom;
Window net_sel_win;

static Atom net_sel_atom;
static Atom net_manager_atom;
static Atom net_message_data_atom;

/* defined in the systray spec */
#define SYSTEM_TRAY_REQUEST_DOCK    0
#define SYSTEM_TRAY_BEGIN_MESSAGE   1
#define SYSTEM_TRAY_CANCEL_MESSAGE  2

static void net_create_selection_window()
{
  net_sel_win = XCreateSimpleWindow(display, root, -1, -1, 1, 1, 0, 0, 0);
  assert(net_sel_win);
}


static void net_destroy_selection_window()
{
  XDestroyWindow(display, net_sel_win);
  net_sel_win = None;
}

void net_icon_remove(TrayWindow *traywin)
{
  assert(traywin);

  XSelectInput(display, traywin->id, NoEventMask);
}


void net_destroy()
{
  net_destroy_selection_window();
}


