/* -*- mode:C; indent-tabs-mode:t; tab-width:8; c-basic-offset:8; -*- */
/* mate-netinfo - A GUI Interface for network utilities
 * Copyright (C) 2003 by William Jon McCann
 * Copyright (C) 2003 by German Poo-Caaman~o
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <string.h>

#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include "finger.h"
#include "utils.h"

void
finger_stop (Netinfo * netinfo)
{
	g_return_if_fail (netinfo != NULL);

	netinfo_stop_process_command (netinfo);
}

void
finger_do (Netinfo * netinfo)
{
	GtkTextBuffer *buffer = NULL;
	GtkTextIter start, end;
	const gchar *host = NULL;
	const gchar *user = NULL;
	gchar *command = NULL;
	gchar *program = NULL;
	gchar *program_name = NULL;
	GtkWidget *parent;

	gchar **command_line;
	gchar **command_options;

	gint i, j, num_terms;
	gchar *command_arg = NULL;
	gboolean user_is_set, host_is_set;

	g_return_if_fail (netinfo != NULL);

	host = netinfo_get_host (netinfo);
	user = netinfo_get_user (netinfo);

	if (g_ascii_strcasecmp (user, "") != 0)
		netinfo->stbar_text =
			g_strdup_printf (_("Getting information of %s on \"%s\""), user,	
					 g_ascii_strcasecmp (host, "") != 0 ? host : "localhost");	
	else
		netinfo->stbar_text =
			g_strdup_printf (_("Getting information of all users on \"%s\""),
					 g_ascii_strcasecmp (host, "") != 0 ? host : "localhost");
	/*if (netinfo->stbar_text)
		g_free (netinfo->stbar_text);
	netinfo->stbar_text = g_strdup_printf (_("Getting information of %s on %s"),
					       g_ascii_strcasecmp (user, "") != 0 ? user : "all users",
					       g_ascii_strcasecmp (host, "") != 0 ? host : "localhost");*/

	buffer =
		gtk_text_view_get_buffer (GTK_TEXT_VIEW (netinfo->output));

	gtk_text_buffer_get_bounds (buffer, &start, &end);
	gtk_text_buffer_delete (buffer, &start, &end);

	parent = gtk_widget_get_toplevel (netinfo->output);
	
	program = util_find_program_in_path ("pinky", NULL);

	if (program != NULL) {
		program_name = g_strdup ("pinky");
	} else {
		program = util_find_program_dialog ("finger", parent);
		program_name = g_strdup ("finger");
	}

	if (program != NULL) {
		/* possible arguments are:
		 * none, user, user@host, @host
		 */
		user_is_set = (strcmp (user, "") != 0);
		host_is_set = (strcmp (host, "") != 0);

		if (user_is_set && ! host_is_set) {
			command_arg = g_strdup (user);
		} else if (user_is_set && host_is_set) {
			command_arg = g_strdup_printf ("%s@%s", user, host);
		} else if (! user_is_set && host_is_set) {
			command_arg = g_strdup_printf ("@%s", host);
		}
		
		num_terms = 3;
		if (command_arg != NULL)
			num_terms++;

		command_options = g_strsplit (FINGER_OPTIONS, " ", -1);
		if (command_options != NULL) {
			for (j = 0; command_options[j] != NULL; j++)
				num_terms++;
		}
		command_line = g_new (gchar *, num_terms + 1);
		i = 0;
		command_line[i++] = g_strdup (program);
		command_line[i++] = g_strdup (program_name);
		if (command_options != NULL) {
			for (j = 0; command_options[j] != NULL; j++)
				command_line[i++] = g_strdup (command_options[j]);
		}
		if (command_arg != NULL)
			command_line[i++] = g_strdup (command_arg);
		command_line[i++] = NULL;

		netinfo->command_line = command_line;

		netinfo_process_command (netinfo);

		g_free (command_arg);
		g_strfreev (command_options);
		g_strfreev (netinfo->command_line);
	}

	g_free (command);
	g_free (program);
}

/* Process each line from finger command */
void
finger_foreach (Netinfo * netinfo, gchar * line, gssize len,
		gpointer user_data)
{
	gchar *text_utf8;
	gsize bytes_written;
	GtkTextBuffer *buffer = NULL;
	GtkTextIter iter;
	
	g_return_if_fail (netinfo != NULL);
	g_return_if_fail (line != NULL);

	buffer =
		gtk_text_view_get_buffer (GTK_TEXT_VIEW (netinfo->output));
	gtk_text_buffer_get_end_iter (buffer, &iter);

	if (len > 0) {
		text_utf8 = g_locale_to_utf8 (line, len,
					      NULL, &bytes_written, NULL);

		gtk_text_buffer_insert
		    (GTK_TEXT_BUFFER (buffer), &iter, text_utf8,
		     bytes_written);
		g_free (text_utf8);
	}
}

void
finger_copy_to_clipboard (Netinfo * netinfo, gpointer user_data)
{
	GtkTextBuffer *buffer = NULL;
	GtkTextIter start, end;
	gchar *result;

	g_return_if_fail (netinfo != NULL);

	buffer =
		gtk_text_view_get_buffer (GTK_TEXT_VIEW (netinfo->output));
	gtk_text_buffer_get_bounds (buffer, &start, &end);
	result = gtk_text_buffer_get_text (buffer, &start, &end, FALSE);

	gtk_clipboard_set_text (gtk_clipboard_get (GDK_NONE), result, -1);
	g_free (result);
}
