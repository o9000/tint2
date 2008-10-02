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


void net_init()
{
  char *name;
  XEvent m;

  name = g_strdup_printf("_NET_SYSTEM_TRAY_S%d", DefaultScreen(display));
  net_sel_atom = XInternAtom(display, name, False);
  assert(net_sel_atom);
  net_opcode_atom = XInternAtom(display, "_NET_SYSTEM_TRAY_OPCODE", False);
  assert(net_opcode_atom);
  net_manager_atom = XInternAtom(display, "MANAGER", False);
  assert(net_manager_atom);
  net_message_data_atom = XInternAtom(display, "_NET_SYSTEM_TRAY_MESSAGE_DATA",
                                      False);
   assert(net_message_data_atom);

  net_create_selection_window();
  
  XSetSelectionOwner(display, net_sel_atom, net_sel_win, CurrentTime);
  if (XGetSelectionOwner(display, net_sel_atom) != net_sel_win)
    return; /* we don't get the selection */

  m.type = ClientMessage;
  m.xclient.message_type = net_manager_atom;
  m.xclient.format = 32;
  m.xclient.data.l[0] = CurrentTime;
  m.xclient.data.l[1] = net_sel_atom;
  m.xclient.data.l[2] = net_sel_win;
  m.xclient.data.l[3] = 0;
  m.xclient.data.l[4] = 0;
  XSendEvent(display, root, False, StructureNotifyMask, &m);
}


void net_destroy()
{
  net_destroy_selection_window();
}


void net_message(XClientMessageEvent *e)
{
  unsigned long opcode;
  Window id;

  assert(e);

  opcode = e->data.l[1];

  switch (opcode)
  {
  case SYSTEM_TRAY_REQUEST_DOCK: /* dock a new icon */
    id = e->data.l[2];
    if (id && icon_add(id, NET))
      XSelectInput(display, id, StructureNotifyMask);
    break;

  case SYSTEM_TRAY_BEGIN_MESSAGE:
    g_printerr("Message From Dockapp\n");
    id = e->window;
    break;

  case SYSTEM_TRAY_CANCEL_MESSAGE:
    g_printerr("Message Cancelled\n");
    id = e->window;
    break;

  default:
    if (opcode == net_message_data_atom) {
      g_printerr("Text For Message From Dockapp:\n%s\n", e->data.b);
      id = e->window;
      break;
    }
    
    /* unknown message type. not in the spec. */
    g_printerr("Warning: Received unknown client message to System Tray "
               "selection window.\n");
    break;
  }
}


void net_icon_remove(TrayWindow *traywin)
{
  assert(traywin);
  
  XSelectInput(display, traywin->id, NoEventMask);
}
