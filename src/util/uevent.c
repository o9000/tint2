/**************************************************************************
*
* Linux Kernel uevent handler
*
* Copyright (C) 2015 Sebastian Reichel <sre@ring0.de>
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License version 2
* or any later version as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**************************************************************************/

#ifdef ENABLE_UEVENT

#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>

#include <linux/types.h>
#include <linux/netlink.h>

#include "common.h"
#include "uevent.h"

static int ueventfd = -1;
static struct sockaddr_nl nls;
static GList *notifiers = NULL;

static const char *has_prefix(const char *str, const char *end, const char *prefix, size_t prefixlen)
{
	if ((end - str) < prefixlen)
		return NULL;

	if (!memcmp(str, prefix, prefixlen))
		return str + prefixlen;

	return NULL;
}

#define HAS_CONST_PREFIX(str, end, prefix) has_prefix((str), end, prefix, sizeof(prefix) - 1)

static void uevent_param_free(gpointer data)
{
	struct uevent_parameter *param = data;
	free(param->key);
	free(param->val);
	free(param);
}

static void uevent_free(struct uevent *ev)
{
	free(ev->path);
	free(ev->subsystem);
	g_list_free_full(ev->params, uevent_param_free);
	free(ev);
}

static struct uevent *uevent_new(char *buffer, int size)
{
	gboolean first = TRUE;

	if (size == 0)
		return NULL;

	struct uevent *ev = calloc(1, sizeof(*ev));
	if (!ev)
		return NULL;

	/* ensure nul termination required by strlen() */
	buffer[size - 1] = '\0';

	const char *s = buffer;
	const char *end = s + size;
	for (; s < end; s += strlen(s) + 1) {
		if (first) {
			const char *p = strchr(s, '@');
			if (!p) {
				/* error: kernel events contain @ */
				/* triggered by udev events, though */
				free(ev);
				return NULL;
			}
			ev->path = strdup(p + 1);
			first = FALSE;
		} else {
			const char *val;
			if ((val = HAS_CONST_PREFIX(s, end, "ACTION=")) != NULL) {
				if (!strcmp(val, "add"))
					ev->action = UEVENT_ADD;
				else if (!strcmp(val, "remove"))
					ev->action = UEVENT_REMOVE;
				else if (!strcmp(val, "change"))
					ev->action = UEVENT_CHANGE;
				else
					ev->action = UEVENT_UNKNOWN;
			} else if ((val = HAS_CONST_PREFIX(s, end, "SEQNUM=")) != NULL) {
				ev->sequence = atoi(val);
			} else if ((val = HAS_CONST_PREFIX(s, end, "SUBSYSTEM=")) != NULL) {
				ev->subsystem = strdup(val);
			} else {
				val = strchr(s, '=');
				if (val) {
					struct uevent_parameter *param = malloc(sizeof(*param));
					if (param) {
						param->key = strndup(s, val - s);
						param->val = strdup(val + 1);
						ev->params = g_list_append(ev->params, param);
					}
				}
			}
		}
	}

	return ev;
}

void uevent_register_notifier(struct uevent_notify *nb)
{
	notifiers = g_list_append(notifiers, nb);
}

void uevent_unregister_notifier(struct uevent_notify *nb)
{
	GList *l = notifiers;

	while (l != NULL) {
		GList *next = l->next;
		struct uevent_notify *lnb = l->data;

		if (memcmp(nb, lnb, sizeof(struct uevent_notify)) == 0)
			notifiers = g_list_delete_link(notifiers, l);

		l = next;
	}
}

void uevent_handler()
{
	if (ueventfd < 0)
		return;

	char buf[512];
	int len = recv(ueventfd, buf, sizeof(buf), MSG_DONTWAIT);
	if (len < 0)
		return;

	struct uevent *ev = uevent_new(buf, len);
	if (ev) {
		for (GList *l = notifiers; l; l = l->next) {
			struct uevent_notify *nb = l->data;

			if (!(ev->action & nb->action))
				continue;

			if (nb->subsystem && strcmp(ev->subsystem, nb->subsystem))
				continue;

			nb->cb(ev, nb->userdata);
		}

		uevent_free(ev);
	}
}

int uevent_init()
{
	/* Open hotplug event netlink socket */
	memset(&nls, 0, sizeof(struct sockaddr_nl));
	nls.nl_family = AF_NETLINK;
	nls.nl_pid = getpid();
	nls.nl_groups = -1;

	/* open socket */
	ueventfd = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);
	if (ueventfd < 0) {
		fprintf(stderr, "Error: socket open failed\n");
		return -1;
	}

	/* Listen to netlink socket */
	if (bind(ueventfd, (void *)&nls, sizeof(struct sockaddr_nl))) {
		fprintf(stderr, "Bind failed\n");
		return -1;
	}

	printf("Kernel uevent interface initialized...\n");

	return ueventfd;
}

void uevent_cleanup()
{
	if (ueventfd >= 0)
		close(ueventfd);
}

#endif
