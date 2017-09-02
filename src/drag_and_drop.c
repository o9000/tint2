/**************************************************************************
* Copyright (C) 2017 tint2 authors
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License version 2
* as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "drag_and_drop.h"
#include "panel.h"
#include "server.h"
#include "task.h"

// Drag and Drop state variables
static Window dnd_source_window;
static Window dnd_target_window;
static int dnd_version;
static Atom dnd_selection;
static Atom dnd_atom;
static int dnd_sent_request;
static LauncherIcon *dnd_launcher_icon;
gboolean debug_dnd = FALSE;

gboolean hidden_panel_shown_for_dnd;

// This fetches all the data from a property
struct Property dnd_read_property(Display *disp, Window w, Atom property)
{
    Atom actual_type;
    int actual_format;
    unsigned long nitems;
    unsigned long bytes_after;
    unsigned char *ret = 0;

    int read_bytes = 1024;

    // Keep trying to read the property until there are no
    // bytes unread.
    do {
        if (ret != 0)
            XFree(ret);
        XGetWindowProperty(disp,
                           w,
                           property,
                           0,
                           read_bytes,
                           False,
                           AnyPropertyType,
                           &actual_type,
                           &actual_format,
                           &nitems,
                           &bytes_after,
                           &ret);
        read_bytes *= 2;
    } while (bytes_after != 0);

    if (debug_dnd)
        fprintf(stderr, "tint2: DnD %s:%d: Property:\n", __FILE__, __LINE__);
    fprintf(stderr, "tint2: DnD %s:%d: Actual type: %s\n", __FILE__, __LINE__, GetAtomName(disp, actual_type));
    fprintf(stderr, "tint2: DnD %s:%d: Actual format: %d\n", __FILE__, __LINE__, actual_format);
    fprintf(stderr, "tint2: DnD %s:%d: Number of items: %lu\n", __FILE__, __LINE__, nitems);

    Property p;
    p.data = ret;
    p.format = actual_format;
    p.nitems = nitems;
    p.type = actual_type;

    return p;
}

// This function takes a list of targets which can be converted to (atom_list, nitems)
// and a list of acceptable targets with prioritees (datatypes). It returns the highest
// entry in datatypes which is also in atom_list: ie it finds the best match.
Atom dnd_pick_target_from_list(Display *disp, Atom *atom_list, int nitems)
{
    Atom to_be_requested = None;
    int i;
    for (i = 0; i < nitems; i++) {
        const char *atom_name = GetAtomName(disp, atom_list[i]);
        fprintf(stderr, "tint2: DnD %s:%d: Type %d = %s\n", __FILE__, __LINE__, i, atom_name);

        // See if this data type is allowed and of higher priority (closer to zero)
        // than the present one.
        if (strcasecmp(atom_name, "STRING") == 0) {
            to_be_requested = atom_list[i];
        } else if (strcasecmp(atom_name, "text/uri-list") == 0 && !to_be_requested) {
            to_be_requested = atom_list[i];
        }
    }

    fprintf(stderr,
            "tint2: DnD %s:%d: Accepting: Type %s\n",
            __FILE__,
            __LINE__,
            GetAtomName(server.display, to_be_requested));

    return to_be_requested;
}

// Finds the best target given up to three atoms provided (any can be None).
// Useful for part of the Xdnd protocol.
Atom dnd_pick_target_from_atoms(Display *disp, Atom t1, Atom t2, Atom t3)
{
    Atom atoms[3];
    int n = 0;

    if (t1 != None)
        atoms[n++] = t1;

    if (t2 != None)
        atoms[n++] = t2;

    if (t3 != None)
        atoms[n++] = t3;

    return dnd_pick_target_from_list(disp, atoms, n);
}

// Finds the best target given a local copy of a property.
Atom dnd_pick_target_from_targets(Display *disp, Property p)
{
    // The list of targets is a list of atoms, so it should have type XA_ATOM
    // but it may have the type TARGETS instead.

    if ((p.type != XA_ATOM && p.type != server.atom.TARGETS) || p.format != 32) {
        // This would be really broken. Targets have to be an atom list
        // and applications should support this. Nevertheless, some
        // seem broken (MATLAB 7, for instance), so ask for STRING
        // next instead as the lowest common denominator
        return XA_STRING;
    } else {
        Atom *atom_list = (Atom *)p.data;

        return dnd_pick_target_from_list(disp, atom_list, p.nitems);
    }
}

void dnd_init()
{
    dnd_source_window = 0;
    dnd_target_window = 0;
    dnd_version = 0;
    dnd_selection = XInternAtom(server.display, "PRIMARY", 0);
    dnd_atom = None;
    dnd_sent_request = 0;
    dnd_launcher_icon = NULL;
    hidden_panel_shown_for_dnd = FALSE;
}

void handle_dnd_enter(XClientMessageEvent *e)
{
    dnd_atom = None;
    int more_than_3 = e->data.l[1] & 1;
    dnd_source_window = e->data.l[0];
    dnd_version = (e->data.l[1] >> 24);

    if (debug_dnd) {
        fprintf(stderr, "tint2: DnD %s:%d: DnDEnter\n", __FILE__, __LINE__);
        fprintf(stderr,
                "DnD %s:%d: DnDEnter. Supports > 3 types = %s\n",
                __FILE__,
                __LINE__,
                more_than_3 ? "yes" : "no");
        fprintf(stderr, "tint2: DnD %s:%d: Protocol version = %d\n", __FILE__, __LINE__, dnd_version);
        fprintf(stderr,
                "tint2: DnD %s:%d: Type 1 = %s\n",
                __FILE__,
                __LINE__,
                GetAtomName(server.display, e->data.l[2]));
        fprintf(stderr,
                "tint2: DnD %s:%d: Type 2 = %s\n",
                __FILE__,
                __LINE__,
                GetAtomName(server.display, e->data.l[3]));
        fprintf(stderr,
                "tint2: DnD %s:%d: Type 3 = %s\n",
                __FILE__,
                __LINE__,
                GetAtomName(server.display, e->data.l[4]));
    }

    // Query which conversions are available and pick the best
    if (more_than_3) {
        // Fetch the list of possible conversions
        // Notice the similarity to TARGETS with paste.
        Property p = dnd_read_property(server.display, dnd_source_window, server.atom.XdndTypeList);
        dnd_atom = dnd_pick_target_from_targets(server.display, p);
        XFree(p.data);
    } else {
        // Use the available list
        dnd_atom = dnd_pick_target_from_atoms(server.display, e->data.l[2], e->data.l[3], e->data.l[4]);
    }

    if (debug_dnd)
        fprintf(stderr,
                "tint2: DnD %s:%d: Requested type = %s\n",
                __FILE__,
                __LINE__,
                GetAtomName(server.display, dnd_atom));
}

void handle_dnd_position(XClientMessageEvent *e)
{
    dnd_target_window = e->window;
    int accept = 0;
    Panel *panel = get_panel(e->window);
    int x, y, mapX, mapY;
    Window child;
    x = (e->data.l[2] >> 16) & 0xFFFF;
    y = e->data.l[2] & 0xFFFF;
    XTranslateCoordinates(server.display, server.root_win, e->window, x, y, &mapX, &mapY, &child);
    Task *task = click_task(panel, mapX, mapY);
    if (task) {
        if (task->desktop != server.desktop)
            change_desktop(task->desktop);
        task_handle_mouse_event(task, TOGGLE);
    } else {
        LauncherIcon *icon = click_launcher_icon(panel, mapX, mapY);
        if (icon) {
            accept = 1;
            dnd_launcher_icon = icon;
        } else {
            dnd_launcher_icon = NULL;
        }
    }

    // send XdndStatus event to get more XdndPosition events
    XClientMessageEvent se;
    se.type = ClientMessage;
    se.window = e->data.l[0];
    se.message_type = server.atom.XdndStatus;
    se.format = 32;
    se.data.l[0] = e->window;      // XID of the target window
    se.data.l[1] = accept ? 1 : 0; // bit 0: accept drop    bit 1: send XdndPosition events if inside rectangle
    se.data.l[2] = 0;              // Rectangle x,y for which no more XdndPosition events
    se.data.l[3] = (1 << 16) | 1;  // Rectangle w,h for which no more XdndPosition events
    if (accept) {
        se.data.l[4] = server.atom.XdndActionCopy;
    } else {
        se.data.l[4] = None; // None = drop will not be accepted
    }

    if (debug_dnd)
        fprintf(stderr,
                "tint2: DnD %s:%d: Accepted: %s\n",
                __FILE__,
                __LINE__,
                accept ? GetAtomName(server.display, (Atom)se.data.l[4]) : "no");

    XSendEvent(server.display, e->data.l[0], False, NoEventMask, (XEvent *)&se);
}

void handle_dnd_drop(XClientMessageEvent *e)
{
    if (dnd_target_window && dnd_launcher_icon) {
        if (dnd_version >= 1) {
            XConvertSelection(server.display,
                              server.atom.XdndSelection,
                              dnd_atom,
                              dnd_selection,
                              dnd_target_window,
                              e->data.l[2]);
        } else {
            XConvertSelection(server.display,
                              server.atom.XdndSelection,
                              dnd_atom,
                              dnd_selection,
                              dnd_target_window,
                              CurrentTime);
        }
    } else {
        // The source is sending anyway, despite instructions to the contrary.
        // So reply that we're not interested.
        XClientMessageEvent m;
        memset(&m, 0, sizeof(m));
        m.type = ClientMessage;
        m.display = e->display;
        m.window = e->data.l[0];
        m.message_type = server.atom.XdndFinished;
        m.format = 32;
        m.data.l[0] = dnd_target_window;
        m.data.l[1] = 0;
        m.data.l[2] = None; // Failed.
        XSendEvent(server.display, e->data.l[0], False, NoEventMask, (XEvent *)&m);
    }
}

GString *tint2_g_string_replace(GString *s, const char *from, const char *to)
{
    GString *result = g_string_new("");
    for (char *p = s->str; *p;) {
        if (strstr(p, from) == p) {
            g_string_append(result, to);
            p += strlen(from);
        } else {
            g_string_append_c(result, *p);
            p += 1;
        }
    }
    g_string_assign(s, result->str);
    g_string_free(result, TRUE);
    return s;
}

void handle_dnd_selection_notify(XSelectionEvent *e)
{
    Atom target = e->target;

    if (debug_dnd) {
        fprintf(stderr, "tint2: DnD %s:%d: A selection notify has arrived!\n", __FILE__, __LINE__);
        fprintf(stderr,
                "DnD %s:%d: Selection atom = %s\n",
                __FILE__,
                __LINE__,
                GetAtomName(server.display, e->selection));
        fprintf(stderr,
                "tint2: DnD %s:%d: Target atom = %s\n",
                __FILE__,
                __LINE__,
                GetAtomName(server.display, target));
        fprintf(stderr,
                "DnD %s:%d: Property atom  = %s\n",
                __FILE__,
                __LINE__,
                GetAtomName(server.display, e->property));
    }

    if (dnd_launcher_icon) {
        Property prop = dnd_read_property(server.display, dnd_target_window, dnd_selection);

        if (prop.data) {
            // If we're being given a list of targets (possible conversions)
            if (target == server.atom.TARGETS && !dnd_sent_request) {
                dnd_sent_request = 1;
                dnd_atom = dnd_pick_target_from_targets(server.display, prop);

                if (dnd_atom == None) {
                    if (debug_dnd)
                        fprintf(stderr, "tint2: No matching datatypes.\n");
                } else {
                    // Request the data type we are able to select
                    if (debug_dnd)
                        fprintf(stderr, "tint2: Now requsting type %s", GetAtomName(server.display, dnd_atom));
                    XConvertSelection(server.display,
                                      dnd_selection,
                                      dnd_atom,
                                      dnd_selection,
                                      dnd_target_window,
                                      CurrentTime);
                }
            } else if (target == dnd_atom) {
                // Dump the binary data
                if (debug_dnd) {
                    fprintf(stderr, "tint2: DnD %s:%d: Received data:\n", __FILE__, __LINE__);
                    fprintf(stderr, "tint2: --------\n");
                    for (int i = 0; i < prop.nitems * prop.format / 8; i++)
                        fprintf(stderr, "%c", ((char *)prop.data)[i]);
                    fprintf(stderr, "tint2: --------\n");
                }

                // TODO: support %r nd %F
                // https://standards.freedesktop.org/desktop-entry-spec/desktop-entry-spec-latest.html#exec-variables
                GString *cmd = g_string_new(dnd_launcher_icon->cmd);

                const char *atom_name = GetAtomName(server.display, prop.type);
                if (strcasecmp(atom_name, "STRING") == 0 ||
                        strcasecmp(atom_name, "text/uri-list") == 0) {
                    GString *url = g_string_new("");
                    GString *prev_url = g_string_new("");
                    for (int i = 0; i < prop.nitems * prop.format / 8; i++) {
                        char c = ((char *)prop.data)[i];
                        if (c == '\n') {
                            // Many programs cannot handle this prefix
                            tint2_g_string_replace(url, "file://", "");
                            // Some programs put duplicates in the list, we remove them
                            if (strcmp(url->str, prev_url->str) != 0) {
                                g_string_append(cmd, " \"");
                                g_string_append(cmd, url->str);
                                g_string_append(cmd, "\"");
                            }
                            g_string_assign(prev_url, url->str);
                            g_string_assign(url, "");
                        } else if (c == '\r') {
                            // Nothing to do
                        } else {
                            if (c == '`' || c == '$' || c == '\\') {
                                g_string_append(url, "\\");
                            }
                            g_string_append_c(url, c);
                        }
                    }
                    g_string_free(url, TRUE);
                    g_string_free(prev_url, TRUE);
                }
                if (debug_dnd)
                    fprintf(stderr, "tint2: DnD %s:%d: Running command: %s\n", __FILE__, __LINE__, cmd->str);
                tint_exec(cmd->str,
                          NULL,
                          NULL,
                          e->time,
                          NULL,
                          0,
                          0,
                          dnd_launcher_icon->start_in_terminal,
                          dnd_launcher_icon->startup_notification);
                g_string_free(cmd, TRUE);

                // Reply OK.
                XClientMessageEvent m;
                memset(&m, 0, sizeof(m));
                m.type = ClientMessage;
                m.display = server.display;
                m.window = dnd_source_window;
                m.message_type = server.atom.XdndFinished;
                m.format = 32;
                m.data.l[0] = dnd_target_window;
                m.data.l[1] = 1;
                m.data.l[2] = server.atom.XdndActionCopy; // We only ever copy.
                XSendEvent(server.display, dnd_source_window, False, NoEventMask, (XEvent *)&m);
                XSync(server.display, False);
            }

            XFree(prop.data);
        }
    }
}
