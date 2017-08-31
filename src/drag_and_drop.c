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
static char *dnd_launcher_exec;
static gboolean dnd_debug = FALSE;

void dnd_init()
{
    dnd_source_window = 0;
    dnd_target_window = 0;
    dnd_version = 0;
    dnd_selection = XInternAtom(server.display, "PRIMARY", 0);
    dnd_atom = None;
    dnd_sent_request = 0;
    dnd_launcher_exec = 0;
}

void handle_dnd_enter(XClientMessageEvent *e)
{
    dnd_atom = None;
    int more_than_3 = e->data.l[1] & 1;
    dnd_source_window = e->data.l[0];
    dnd_version = (e->data.l[1] >> 24);

    if (dnd_debug) {
        fprintf(stderr, "DnD %s:%d: DnDEnter\n", __FILE__, __LINE__);
        fprintf(stderr,
                "DnD %s:%d: DnDEnter. Supports > 3 types = %s\n",
                __FILE__,
                __LINE__,
                more_than_3 ? "yes" : "no");
        fprintf(stderr, "DnD %s:%d: Protocol version = %d\n", __FILE__, __LINE__, dnd_version);
        fprintf(stderr, "DnD %s:%d: Type 1 = %s\n", __FILE__, __LINE__, GetAtomName(server.display, e->data.l[2]));
        fprintf(stderr, "DnD %s:%d: Type 2 = %s\n", __FILE__, __LINE__, GetAtomName(server.display, e->data.l[3]));
        fprintf(stderr, "DnD %s:%d: Type 3 = %s\n", __FILE__, __LINE__, GetAtomName(server.display, e->data.l[4]));
    }

    // Query which conversions are available and pick the best
    if (more_than_3) {
        // Fetch the list of possible conversions
        // Notice the similarity to TARGETS with paste.
        Property p = read_property(server.display, dnd_source_window, server.atom.XdndTypeList);
        dnd_atom = pick_target_from_targets(server.display, p);
        XFree(p.data);
    } else {
        // Use the available list
        dnd_atom = pick_target_from_atoms(server.display, e->data.l[2], e->data.l[3], e->data.l[4]);
    }

    if (dnd_debug)
        fprintf(stderr, "DnD %s:%d: Requested type = %s\n", __FILE__, __LINE__, GetAtomName(server.display, dnd_atom));
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
            dnd_launcher_exec = icon->cmd;
        } else {
            dnd_launcher_exec = 0;
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
        se.data.l[4] = dnd_version >= 2 ? e->data.l[4] : server.atom.XdndActionCopy;
    } else {
        se.data.l[4] = None; // None = drop will not be accepted
    }

    XSendEvent(server.display, e->data.l[0], False, NoEventMask, (XEvent *)&se);
}

void handle_dnd_drop(XClientMessageEvent *e)
{
    if (dnd_target_window && dnd_launcher_exec) {
        if (dnd_version >= 1) {
            XConvertSelection(server.display,
                              server.atom.XdndSelection,
                              XA_STRING,
                              dnd_selection,
                              dnd_target_window,
                              e->data.l[2]);
        } else {
            XConvertSelection(server.display,
                              server.atom.XdndSelection,
                              XA_STRING,
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

void handle_dnd_selection_notify(XSelectionEvent *e)
{
    Atom target = e->target;

    if (dnd_debug) {
        fprintf(stderr, "DnD %s:%d: A selection notify has arrived!\n", __FILE__, __LINE__);
        fprintf(stderr,
                "DnD %s:%d: Selection atom = %s\n",
                __FILE__,
                __LINE__,
                GetAtomName(server.display, e->selection));
        fprintf(stderr, "DnD %s:%d: Target atom    = %s\n", __FILE__, __LINE__, GetAtomName(server.display, target));
        fprintf(stderr,
                "DnD %s:%d: Property atom  = %s\n",
                __FILE__,
                __LINE__,
                GetAtomName(server.display, e->property));
    }

    if (e->property != None && dnd_launcher_exec) {
        Property prop = read_property(server.display, dnd_target_window, dnd_selection);

        // If we're being given a list of targets (possible conversions)
        if (target == server.atom.TARGETS && !dnd_sent_request) {
            dnd_sent_request = 1;
            dnd_atom = pick_target_from_targets(server.display, prop);

            if (dnd_atom == None) {
                if (dnd_debug)
                    fprintf(stderr, "No matching datatypes.\n");
            } else {
                // Request the data type we are able to select
                if (dnd_debug)
                    fprintf(stderr, "Now requsting type %s", GetAtomName(server.display, dnd_atom));
                XConvertSelection(server.display,
                                  dnd_selection,
                                  dnd_atom,
                                  dnd_selection,
                                  dnd_target_window,
                                  CurrentTime);
            }
        } else if (target == dnd_atom) {
            // Dump the binary data
            if (dnd_debug) {
                fprintf(stderr, "DnD %s:%d: Data begins:\n", __FILE__, __LINE__);
                fprintf(stderr, "--------\n");
                for (int i = 0; i < prop.nitems * prop.format / 8; i++)
                    fprintf(stderr, "%c", ((char *)prop.data)[i]);
                fprintf(stderr, "--------\n");
            }

            int cmd_length = 0;
            cmd_length += 1;                             // (
            cmd_length += strlen(dnd_launcher_exec) + 1; // exec + space
            cmd_length += 1;                             // open double quotes
            for (int i = 0; i < prop.nitems * prop.format / 8; i++) {
                char c = ((char *)prop.data)[i];
                if (c == '\n') {
                    if (i < prop.nitems * prop.format / 8 - 1) {
                        cmd_length += 3; // close double quotes, space, open double quotes
                    }
                } else if (c == '\r') {
                    // Nothing to do
                } else {
                    cmd_length += 1; // 1 character
                    if (c == '`' || c == '$' || c == '\\') {
                        cmd_length += 1; // escape with one backslash
                    }
                }
            }
            cmd_length += 1; // close double quotes
            cmd_length += 2; // &)
            cmd_length += 1; // terminator

            char *cmd = (char *)calloc(cmd_length, 1);
            cmd[0] = '\0';
            strcat(cmd, "(");
            strcat(cmd, dnd_launcher_exec);
            strcat(cmd, " \"");
            for (int i = 0; i < prop.nitems * prop.format / 8; i++) {
                char c = ((char *)prop.data)[i];
                if (c == '\n') {
                    if (i < prop.nitems * prop.format / 8 - 1) {
                        strcat(cmd, "\" \"");
                    }
                } else if (c == '\r') {
                    // Nothing to do
                } else {
                    if (c == '`' || c == '$' || c == '\\') {
                        strcat(cmd, "\\");
                    }
                    char sc[2];
                    sc[0] = c;
                    sc[1] = '\0';
                    strcat(cmd, sc);
                }
            }
            strcat(cmd, "\"");
            strcat(cmd, "&)");
            if (dnd_debug)
                fprintf(stderr, "DnD %s:%d: Running command: %s\n", __FILE__, __LINE__, cmd);
            tint_exec(cmd, NULL, NULL, e->time, NULL, 0, 0);
            free(cmd);

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
