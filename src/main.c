/* -*- mode:C; indent-tabs-mode:t; tab-width:8; c-basic-offset:8; -*- */

/* mate-netinfo - A GUI Interface for network utilities
 * Copyright (C) 2002, 2003 by German Poo-Caaman~o
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <string.h>
#include <glib/gi18n.h>

#include <glibtop.h>

#include "callbacks.h"
#include "ping.h"
#include "traceroute.h"
#include "info.h"
#include "netstat.h"
#include "scan.h"
#include "lookup.h"
#include "finger.h"
#include "whois.h"
#include "utils.h"
#include "gn-combo-history.h"

Netinfo *load_ping_widgets_from_builder (GtkBuilder * builder);
Netinfo *load_traceroute_widgets_from_builder (GtkBuilder * builder);
Netinfo *load_netstat_widgets_from_builder (GtkBuilder * builder);
Netinfo *load_scan_widgets_from_builder (GtkBuilder * builder);
Netinfo *load_lookup_widgets_from_builder (GtkBuilder * builder);
Netinfo *load_finger_widgets_from_builder (GtkBuilder * builder);
Netinfo *load_whois_widgets_from_builder (GtkBuilder * builder);
Netinfo *load_info_widgets_from_builder (GtkBuilder * builder);
static gboolean start_initial_process_cb (gpointer data);

int
main (int argc, char *argv[])
{
	GtkWidget *window;
	GtkWidget *menu_beep;
	GtkBuilder *builder;
	GtkWidget *notebook;
	GtkWidget *statusbar;
    GtkUIManager *ui;
	const gchar *dialog = UI_DIR "mate-nettool.ui";
	Netinfo *pinger;
	Netinfo *tracer;
	Netinfo *netstat;
	Netinfo *info;
	Netinfo *scan;
	Netinfo *lookup;
	Netinfo *finger;
	Netinfo *whois;
	gint current_page = 0;
	static gchar *info_input = NULL;
	static gchar *ping_input = NULL;
	static gchar *netstat_input = NULL;
	static gchar *scan_input = NULL;
	static gchar *traceroute_input = NULL;
	static gchar *lookup_input = NULL;
	static gchar *finger_input = NULL;
	static gchar *whois_input = NULL;
	GError *error = NULL;

	GOptionEntry options[] = {
		{ "info", 'i', 0, G_OPTION_ARG_STRING, &info_input,
 		  N_("Load information for a network device"),
 		  N_("DEVICE") },

		{ "ping", 'p', 0, G_OPTION_ARG_STRING, &ping_input,
 		  N_("Send a ping to a network address"),
 		  N_("HOST") },

		{ "netstat", 'n', 0, G_OPTION_ARG_STRING, &netstat_input,
 		  N_("Get netstat information.  Valid options are: route, active, multicast."),
 		  N_("COMMAND") },

		{ "traceroute", 't', 0, G_OPTION_ARG_STRING, &traceroute_input,
 		  N_("Trace a route to a network address"),
 		  N_("HOST") },

		{ "port-scan", 's', 0, G_OPTION_ARG_STRING, &scan_input,
 		  N_("Port scan a network address"),
 		  N_("HOST") },

		{ "lookup", 'l', 0, G_OPTION_ARG_STRING, &lookup_input,
 		  N_("Look up a network address"),
 		  N_("HOST") },

		{ "finger", 'f', 0, G_OPTION_ARG_STRING, &finger_input,
 		  N_("Finger command to run"),
 		  N_("USER") },

		{ "whois", 'w', 0, G_OPTION_ARG_STRING, &whois_input,
 		  N_("Perform a whois lookup for a network domain"),
 		  N_("DOMAIN") },

		{ NULL, '\0', 0, 0, NULL, NULL, NULL }
 	};

#ifdef ENABLE_NLS
	bindtextdomain (GETTEXT_PACKAGE, MATE_NETTOOL_LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
#endif

	glibtop_init ();

	if (!gtk_init_with_args (&argc, &argv, NULL, options, NULL, &error)) {
		g_print ("%s\n\n", error->message);
		return -1;
	}

	if (!g_file_test (dialog, G_FILE_TEST_EXISTS)) {
		g_critical (_("The file %s doesn't exist, "
			      "please check if mate-nettool is correctly installed"),
			    dialog);
		return -1;
	}

	gtk_window_set_default_icon_name ("mate-nettool");
	
	builder = gtk_builder_new ();
        gtk_builder_add_from_file (builder, dialog, NULL);
	window = GTK_WIDGET (gtk_builder_get_object (builder, "main_window"));
	statusbar = GTK_WIDGET (gtk_builder_get_object (builder, "statusbar"));
	gtk_statusbar_push (GTK_STATUSBAR (statusbar), 0, _("Idle"));

	g_signal_connect (G_OBJECT (window), "delete-event",
			  G_CALLBACK (gn_quit_app), NULL);
	
	pinger = load_ping_widgets_from_builder (builder);
	tracer = load_traceroute_widgets_from_builder (builder);
	netstat = load_netstat_widgets_from_builder (builder);
	info = load_info_widgets_from_builder (builder);
	scan = load_scan_widgets_from_builder (builder);
	lookup = load_lookup_widgets_from_builder (builder);
	finger = load_finger_widgets_from_builder (builder);
	whois = load_whois_widgets_from_builder (builder);

	if (info_input) {
		current_page = INFO;
		info_set_nic (info, info_input);
	}
	if (ping_input) {
		current_page = PING;
		netinfo_set_host (pinger, ping_input);
		g_idle_add (start_initial_process_cb, pinger);
	}
	if (netstat_input) {
		current_page = NETSTAT;
		if (! strcmp (netstat_input, "route"))
			gtk_toggle_button_set_active
				(GTK_TOGGLE_BUTTON (netstat->routing), TRUE);
		else if (! strcmp (netstat_input, "active"))
			gtk_toggle_button_set_active
				(GTK_TOGGLE_BUTTON (netstat->protocol), TRUE);
		else if (! strcmp (netstat_input, "multicast"))
			gtk_toggle_button_set_active
				(GTK_TOGGLE_BUTTON (netstat->multicast), TRUE);
		g_idle_add (start_initial_process_cb, netstat);
	}
	if (traceroute_input) {
		current_page = TRACEROUTE;
		netinfo_set_host (tracer, traceroute_input);
		g_idle_add (start_initial_process_cb, tracer);
	}
	if (scan_input) {
		current_page = PORTSCAN;
		netinfo_set_host (scan, scan_input);
		g_idle_add (start_initial_process_cb, scan);
	}
	if (lookup_input) {
		current_page = LOOKUP;
		netinfo_set_host (lookup, lookup_input);
		g_idle_add (start_initial_process_cb, lookup);
	}
	if (finger_input) {
		gchar **split_input = NULL;
		current_page = FINGER;
		split_input = g_strsplit (finger_input, "@", 2);
		if (split_input[0])
			netinfo_set_user (finger, split_input[0]);
		if (split_input[1])
			netinfo_set_host (finger, split_input[1]);
		g_strfreev (split_input);
		g_idle_add (start_initial_process_cb, finger);
	}
	if (whois_input) {
		current_page = WHOIS;
		netinfo_set_host (whois, whois_input);
		g_idle_add (start_initial_process_cb, whois);
	}

	notebook = GTK_WIDGET (gtk_builder_get_object (builder, "notebook"));
	g_object_set_data (G_OBJECT (notebook), "pinger", pinger);
	g_object_set_data (G_OBJECT (notebook), "tracer", tracer);
	g_object_set_data (G_OBJECT (notebook), "netstat", netstat);
	g_object_set_data (G_OBJECT (notebook), "info", info);
	g_object_set_data (G_OBJECT (notebook), "scan", scan);
	g_object_set_data (G_OBJECT (notebook), "lookup", lookup);
	g_object_set_data (G_OBJECT (notebook), "finger", finger);
	g_object_set_data (G_OBJECT (notebook), "whois", whois);
	
	menu_beep = GTK_WIDGET (gtk_builder_get_object (builder, "m_beep"));
	ui = GTK_UI_MANAGER (gtk_builder_get_object (builder, "uimanager1"));
	g_object_ref (ui);

	g_signal_connect (G_OBJECT (menu_beep), "activate",
			  G_CALLBACK (on_beep_activate),
			  (gpointer) pinger); 
	
	gtk_builder_connect_signals (builder, NULL);
	g_object_unref (G_OBJECT (builder));

	gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), current_page);

	gtk_widget_show (window);

	gtk_main ();

	glibtop_close ();

	g_free (pinger);
	g_free (tracer);
	g_free (netstat);
	g_free (info);
	g_free (scan);
	g_free (lookup);
	g_free (finger);
	g_free (whois);

	return 0;
}

static gboolean
start_initial_process_cb (gpointer data)
{
	Netinfo *ni = data;
	NetinfoActivateFn fn_cb;

	g_return_val_if_fail (data != NULL, FALSE);

	fn_cb = (NetinfoActivateFn) ni->button_callback;
	if (fn_cb)
		(*fn_cb) (ni->button, data);
	return FALSE;
}

/* The value returned must be released from memory */
Netinfo *
load_ping_widgets_from_builder (GtkBuilder * builder)
{
	Netinfo *pinger;
	GtkWidget *vbox_ping;
	GtkWidget *label;
	GtkEntry  *entry_host;
	GtkTreeModel *model;
	GtkEntryCompletion *completion;

	g_return_val_if_fail (builder != NULL, NULL);

	pinger = g_new0 (Netinfo, 1);

	pinger->main_window = GTK_WIDGET (gtk_builder_get_object (builder, "main_window"));
	pinger->progress_bar = GTK_WIDGET (gtk_builder_get_object (builder, "progress_bar"));
	pinger->page_label = GTK_WIDGET (gtk_builder_get_object (builder, "ping"));
	pinger->running = FALSE;
	pinger->child_pid = 0;
	pinger->host = GTK_WIDGET (gtk_builder_get_object (builder, "ping_host"));
	pinger->count = GTK_WIDGET (gtk_builder_get_object (builder, "ping_count"));
	pinger->output = GTK_WIDGET (gtk_builder_get_object (builder, "ping_output"));
	pinger->limited = GTK_WIDGET (gtk_builder_get_object (builder, "ping_limited"));
	pinger->button = GTK_WIDGET (gtk_builder_get_object (builder, "ping_button"));
	pinger->graph = GTK_WIDGET (gtk_builder_get_object (builder, "ping_graph"));
	pinger->sensitive = pinger->host;
	pinger->label_run = _("Ping");
	pinger->label_stop = NULL;
	pinger->routing = NULL;
	pinger->protocol = NULL;
	pinger->multicast = NULL;
	pinger->min = GTK_WIDGET (gtk_builder_get_object (builder, "ping_minimum"));
	pinger->avg = GTK_WIDGET (gtk_builder_get_object (builder, "ping_average"));
	pinger->max = GTK_WIDGET (gtk_builder_get_object (builder, "ping_maximum"));
	pinger->packets_transmitted = GTK_WIDGET (gtk_builder_get_object (builder, "ping_packets_transmitted"));
	pinger->packets_received = GTK_WIDGET (gtk_builder_get_object (builder, "ping_packets_received"));
	pinger->packets_success = GTK_WIDGET (gtk_builder_get_object (builder, "ping_packets_success"));

	pinger->status_bar = GTK_WIDGET (gtk_builder_get_object (builder, "statusbar"));
	pinger->stbar_text = NULL;

	vbox_ping = GTK_WIDGET (gtk_builder_get_object (builder, "vbox_ping"));

	label = GTK_WIDGET (gtk_builder_get_object (builder, "ping_host_label"));
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), pinger->host);
	
	pinger->button_callback = G_CALLBACK (on_ping_activate);
	pinger->process_line = NETINFO_FOREACH_FUNC (ping_foreach_with_tree);
	pinger->copy_output = NETINFO_COPY_FUNC (ping_copy_to_clipboard);

	model = GTK_TREE_MODEL (gtk_list_store_new (1, G_TYPE_STRING));
	gtk_combo_box_set_model (GTK_COMBO_BOX (pinger->host), model);
	g_object_unref (model);

	/*gtk_combo_box_entry_set_text_column (GTK_COMBO_BOX_ENTRY (pinger->host), 0);*/

	entry_host = GTK_ENTRY (gtk_bin_get_child (GTK_BIN (pinger->host)));
	
	completion = gtk_entry_completion_new ();
	gtk_entry_set_completion (entry_host, completion);
	g_object_unref (completion);
	gtk_entry_completion_set_model (completion, model);
	gtk_entry_completion_set_text_column (completion, 0);
	g_object_unref (model);

	pinger->history = gn_combo_history_new ();
	gn_combo_history_set_id (pinger->history, "MATE_Network_netinfo_host");
	gn_combo_history_set_combo (pinger->history, GTK_COMBO_BOX (pinger->host));

	g_signal_connect (G_OBJECT (entry_host), "activate",
			  G_CALLBACK (on_ping_activate),
			  pinger);
	g_signal_connect (G_OBJECT (pinger->limited), "toggled",
			  G_CALLBACK (on_ping_toggled),
			  pinger);
	g_signal_connect (G_OBJECT (pinger->button), "clicked",
			  pinger->button_callback,
			  pinger);
	g_signal_connect (G_OBJECT (pinger->graph), "expose-event",
			  G_CALLBACK (on_ping_graph_expose),
			  pinger);

	return pinger;
}

/* The value returned must be released from memory */
Netinfo *
load_traceroute_widgets_from_builder (GtkBuilder * builder)
{
	Netinfo *tracer;
	GtkWidget *vbox_traceroute;
	GtkWidget *label;
	GtkEntry  *entry_host;
	GtkTreeModel *model;
	GtkEntryCompletion *completion;

	g_return_val_if_fail (builder != NULL, NULL);

	tracer = g_new0 (Netinfo, 1);

	tracer->main_window = GTK_WIDGET (gtk_builder_get_object (builder, "main_window"));
	tracer->progress_bar = GTK_WIDGET (gtk_builder_get_object (builder, "progress_bar"));
	tracer->page_label = GTK_WIDGET (gtk_builder_get_object (builder, "traceroute"));
	tracer->running = FALSE;
	tracer->child_pid = 0;
	tracer->host = GTK_WIDGET (gtk_builder_get_object (builder, "traceroute_host"));
	tracer->output = GTK_WIDGET (gtk_builder_get_object (builder, "traceroute_output"));
	tracer->button = GTK_WIDGET (gtk_builder_get_object (builder, "traceroute_button"));
	tracer->count = NULL;
	tracer->limited = NULL;
	tracer->sensitive = tracer->host;
	tracer->label_run = _("Trace");
	tracer->label_stop = NULL;
	tracer->routing = NULL;
	tracer->protocol = NULL;
	tracer->multicast = NULL;

	tracer->status_bar = GTK_WIDGET (gtk_builder_get_object (builder, "statusbar"));
	tracer->stbar_text = NULL;
	
	vbox_traceroute = GTK_WIDGET (gtk_builder_get_object (builder, "vbox_traceroute"));

	label = GTK_WIDGET (gtk_builder_get_object (builder, "traceroute_host_label"));
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), tracer->host);

	tracer->button_callback = G_CALLBACK (on_traceroute_activate);
	tracer->process_line = NETINFO_FOREACH_FUNC (traceroute_foreach_with_tree);
	tracer->copy_output = NETINFO_COPY_FUNC (traceroute_copy_to_clipboard);

	model = GTK_TREE_MODEL (gtk_list_store_new (1, G_TYPE_STRING));
	gtk_combo_box_set_model (GTK_COMBO_BOX (tracer->host), model);
	g_object_unref (model);

	/*gtk_combo_box_entry_set_text_column (GTK_COMBO_BOX_ENTRY (tracer->host), 0);*/

	entry_host = GTK_ENTRY (gtk_bin_get_child (GTK_BIN (tracer->host)));

	completion = gtk_entry_completion_new ();
	gtk_entry_set_completion (entry_host, completion);
	g_object_unref (completion);
	gtk_entry_completion_set_model (completion, model);
	gtk_entry_completion_set_text_column (completion, 0);
	g_object_unref (model);

	tracer->history = gn_combo_history_new ();
	gn_combo_history_set_id (tracer->history, "MATE_Network_netinfo_host");
	gn_combo_history_set_combo (tracer->history, GTK_COMBO_BOX (tracer->host));

	g_signal_connect (G_OBJECT (entry_host), "activate",
			  G_CALLBACK (on_traceroute_activate),
			  tracer);
	g_signal_connect (G_OBJECT (tracer->button), "clicked",
			  tracer->button_callback,
			  tracer);

	return tracer;
}

Netinfo *
load_netstat_widgets_from_builder (GtkBuilder * builder)
{
	Netinfo *netstat;
	GtkWidget *vbox_netstat;

	g_return_val_if_fail (builder != NULL, NULL);

	netstat = g_new0 (Netinfo, 1);

	netstat->main_window = GTK_WIDGET (gtk_builder_get_object (builder, "main_window"));
	netstat->progress_bar = GTK_WIDGET (gtk_builder_get_object (builder, "progress_bar"));
	netstat->page_label = GTK_WIDGET (gtk_builder_get_object (builder, "netstat"));
	netstat->running = FALSE;
	netstat->child_pid = 0;
	netstat->host = NULL;
	netstat->count = NULL;
	netstat->output = GTK_WIDGET (gtk_builder_get_object (builder, "netstat_output"));
	netstat->limited = NULL;
	netstat->button = GTK_WIDGET (gtk_builder_get_object (builder, "netstat_button"));
	netstat->routing = GTK_WIDGET (gtk_builder_get_object (builder, "netstat_routing"));
	netstat->protocol = GTK_WIDGET (gtk_builder_get_object (builder, "netstat_protocol"));
	netstat->multicast = GTK_WIDGET (gtk_builder_get_object (builder, "netstat_multicast"));
	netstat->sensitive = NULL;
	netstat->label_run = _("Netstat");
	netstat->label_stop = NULL;

	netstat->status_bar = GTK_WIDGET (gtk_builder_get_object (builder, "statusbar"));
	netstat->stbar_text = NULL;
	
	vbox_netstat = GTK_WIDGET (gtk_builder_get_object (builder, "vbox_netstat"));
	
	netstat->button_callback = G_CALLBACK (on_netstat_activate);
	netstat->process_line = NETINFO_FOREACH_FUNC (netstat_foreach_with_tree);
	netstat->copy_output = NETINFO_COPY_FUNC (netstat_copy_to_clipboard);	
	
	g_signal_connect (G_OBJECT (netstat->button), "clicked",
				  netstat->button_callback,
				  netstat);
/*
	g_signal_connect (G_OBJECT (netstat->protocol), "toggled",
				  G_CALLBACK (on_protocol_button_toggled),
				  netstat);
	g_signal_connect (G_OBJECT (netstat->routing), "toggled",
				  G_CALLBACK (on_protocol_button_toggled),
				  netstat);
	g_signal_connect (G_OBJECT (netstat->multicast), "toggled",
				  G_CALLBACK (on_protocol_button_toggled),
				  netstat);
*/
	return netstat;
}

static void
info_list_ip_addr_add_columns (GtkWidget *list_ip_addr)
{
	GtkTreeViewColumn *column;
	GtkCellRenderer   *renderer;

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Protocol"),
							   renderer,
							   "text", 0,
							   NULL);
	gtk_tree_view_insert_column (GTK_TREE_VIEW (list_ip_addr), column, 0);

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("IP Address"),
							   renderer,
							   "text", 1,
							   NULL);
	gtk_tree_view_insert_column (GTK_TREE_VIEW (list_ip_addr), column, 1);

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Netmask / Prefix"),
							   renderer,
							   "text", 2,
							   NULL);
	gtk_tree_view_insert_column (GTK_TREE_VIEW (list_ip_addr), column, 2);
	
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Broadcast"),
							   renderer,
							   "text", 3,
							   NULL);
	gtk_tree_view_insert_column (GTK_TREE_VIEW (list_ip_addr), column, 3);
	
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Scope"),
							   renderer,
							   "text", 4,
							   NULL);
	gtk_tree_view_insert_column (GTK_TREE_VIEW (list_ip_addr), column, 4);

	
}

/* The value returned must be released from memory */
Netinfo *
load_info_widgets_from_builder (GtkBuilder * builder)
{
	Netinfo      *info;
	GtkTreeModel *model;
	GtkWidget    *label1;

	g_return_val_if_fail (builder != NULL, NULL);

	info = g_malloc (sizeof (Netinfo));

	info->main_window = GTK_WIDGET (gtk_builder_get_object (builder, "main_window"));
	info->running = FALSE;
	info->combo = GTK_WIDGET (gtk_builder_get_object (builder, "info_combo"));
	info->ipv6_frame = GTK_WIDGET (gtk_builder_get_object (builder, "info_ipv6_frame"));
	info->progress_bar = GTK_WIDGET (gtk_builder_get_object (builder, "progress_bar"));
	info->page_label = GTK_WIDGET (gtk_builder_get_object (builder, "device"));
	info->hw_address = GTK_WIDGET (gtk_builder_get_object (builder, "info_hw_address"));
	info->ip_address = GTK_WIDGET (gtk_builder_get_object (builder, "info_ip_address"));
	info->netmask = GTK_WIDGET (gtk_builder_get_object (builder, "info_netmask"));
	info->broadcast = GTK_WIDGET (gtk_builder_get_object (builder, "info_broadcast"));
	info->multicast = GTK_WIDGET (gtk_builder_get_object (builder, "info_multicast"));
	info->link_speed = GTK_WIDGET (gtk_builder_get_object (builder, "info_link_speed"));
	info->state = GTK_WIDGET (gtk_builder_get_object (builder, "info_state"));
	info->mtu = GTK_WIDGET (gtk_builder_get_object (builder, "info_mtu"));
	info->tx_bytes = GTK_WIDGET (gtk_builder_get_object (builder, "info_tx_bytes"));
	info->tx = GTK_WIDGET (gtk_builder_get_object (builder, "info_tx"));
	info->tx_errors = GTK_WIDGET (gtk_builder_get_object (builder, "info_tx_errors"));
	info->rx_bytes = GTK_WIDGET (gtk_builder_get_object (builder, "info_rx_bytes"));
	info->rx = GTK_WIDGET (gtk_builder_get_object (builder, "info_rx"));
	info->rx_errors = GTK_WIDGET (gtk_builder_get_object (builder, "info_rx_errors"));
	info->collisions = GTK_WIDGET (gtk_builder_get_object (builder, "info_collisions"));
	info->list_ip_addr = GTK_WIDGET (gtk_builder_get_object (builder, "info_list_ip_addr"));
	info->configure_button = GTK_WIDGET (gtk_builder_get_object (builder, "info_configure_button"));

	info->status_bar = GTK_WIDGET (gtk_builder_get_object (builder, "statusbar"));
	info->stbar_text = NULL;

	info->network_tool_path = util_find_program_in_path (GST_NETWORK_TOOL, NULL);

	model = GTK_TREE_MODEL (gtk_list_store_new (5, G_TYPE_STRING, G_TYPE_STRING,
						    G_TYPE_STRING, G_TYPE_STRING,
						    G_TYPE_STRING));
	gtk_tree_view_set_model (GTK_TREE_VIEW (info->list_ip_addr), model);
	g_object_unref (model);
	
	info_list_ip_addr_add_columns (info->list_ip_addr);

	label1 = GTK_WIDGET (gtk_builder_get_object (builder, "info_combo_label"));
	gtk_label_set_mnemonic_widget (GTK_LABEL (label1), info->combo);

	model = GTK_TREE_MODEL (gtk_list_store_new (3, GDK_TYPE_PIXBUF,
						    G_TYPE_STRING,
						    G_TYPE_POINTER));
	gtk_combo_box_set_model (GTK_COMBO_BOX (info->combo), model);

	g_object_unref (model);

	g_signal_connect (G_OBJECT (info->configure_button), "clicked",
			  G_CALLBACK (on_configure_button_clicked),
			  info);

	g_signal_connect (G_OBJECT (info->combo), "changed",
			  G_CALLBACK (info_nic_changed),
			  info);
	
	info_load_iface (info);
	info->copy_output = NETINFO_COPY_FUNC (info_copy_to_clipboard);

	return info;
}

Netinfo *
load_scan_widgets_from_builder (GtkBuilder * builder)
{
	Netinfo *scan;
	GtkEntry  *entry_host;
	GtkWidget *label;
	GtkTreeModel *model;
	GtkEntryCompletion *completion;

	g_return_val_if_fail (builder != NULL, NULL);

	scan = g_new0 (Netinfo, 1);

	scan->main_window = GTK_WIDGET (gtk_builder_get_object (builder, "main_window"));
	scan->progress_bar = GTK_WIDGET (gtk_builder_get_object (builder, "progress_bar"));
	scan->page_label = GTK_WIDGET (gtk_builder_get_object (builder, "scan"));
	scan->running = FALSE;
	scan->child_pid = 0;
	scan->host = GTK_WIDGET (gtk_builder_get_object (builder, "scan_host"));
	scan->count = NULL;
	scan->output = GTK_WIDGET (gtk_builder_get_object (builder, "scan_output"));
	scan->limited = NULL;
	scan->button = GTK_WIDGET (gtk_builder_get_object (builder, "scan_button"));
	scan->routing = NULL;
	scan->protocol = NULL;
	scan->multicast = NULL;
	scan->sensitive = NULL;
	scan->label_run = _("Scan");
	scan->label_stop = NULL;

	scan->status_bar = GTK_WIDGET (gtk_builder_get_object (builder, "statusbar"));
	scan->stbar_text = NULL;
	
	label = GTK_WIDGET (gtk_builder_get_object (builder, "scan_host_label"));
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), scan->host);

	scan->button_callback = G_CALLBACK (on_scan_activate);
	scan->copy_output = NETINFO_COPY_FUNC (scan_copy_to_clipboard);
	scan->process_line = NETINFO_FOREACH_FUNC (scan_foreach);

	model = GTK_TREE_MODEL (gtk_list_store_new (1, G_TYPE_STRING));
	gtk_combo_box_set_model (GTK_COMBO_BOX (scan->host), model);
	g_object_unref (model);

	/*gtk_combo_box_entry_set_text_column (GTK_COMBO_BOX_ENTRY (scan->host), 0);*/

	entry_host = GTK_ENTRY (gtk_bin_get_child (GTK_BIN (scan->host)));

	completion = gtk_entry_completion_new ();
	gtk_entry_set_completion (entry_host, completion);
	g_object_unref (completion);
	gtk_entry_completion_set_model (completion, model);
	gtk_entry_completion_set_text_column (completion, 0);
	g_object_unref (model);

	scan->history = gn_combo_history_new ();
	gn_combo_history_set_id (scan->history, "MATE_Network_netinfo_host");
	gn_combo_history_set_combo (scan->history, GTK_COMBO_BOX (scan->host));

	g_signal_connect (G_OBJECT (entry_host), "activate",
			  scan->button_callback,
			  scan);
	g_signal_connect (G_OBJECT (scan->button), "clicked",
                          scan->button_callback,
                          scan);

	return scan;
}

static void
nettool_lookup_setup_combo_type (Netinfo *lookup)
{
	gint i;
	GtkTreeModel *model;
	GtkTreeIter   iter;
	GtkCellRenderer *renderer;
	gchar *types[] = {
		N_("Default Information"),
		N_("Internet Address"),
		N_("Canonical Name"),
		N_("CPU / OS Type"),
		/* When asking for MX record in DNS context */
		N_("Mailbox Exchange"),
		N_("Mailbox Information"),
		/* When asking for NS record in DNS context */
		N_("Name Server"),
		N_("Host name for Address"),
		N_("Start-of-authority"),
		N_("Text Information"),
		N_("Well Known Services"),
		N_("Any / All Information"),
		NULL
	};

	model = GTK_TREE_MODEL (gtk_list_store_new (1, G_TYPE_STRING));
	
	for (i=0; types[i]; i++) {
		gtk_list_store_append (GTK_LIST_STORE (model), &iter);
		gtk_list_store_set (GTK_LIST_STORE (model), &iter,
				    0, _(types[i]), -1);
	}
	
	gtk_combo_box_set_model (GTK_COMBO_BOX (lookup->type), model);

	g_object_unref (model);

	gtk_cell_layout_clear (GTK_CELL_LAYOUT (lookup->type));
	renderer = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (lookup->type), renderer, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (lookup->type), renderer,
					"text", 0, NULL);

	gtk_combo_box_set_active (GTK_COMBO_BOX (lookup->type), 0);
}

/* The value returned must be released from memory */
Netinfo *
load_lookup_widgets_from_builder (GtkBuilder * builder)
{
	Netinfo *lookup;
	GtkWidget *vbox_lookup;
	GtkWidget *label;
	GtkEntry  *entry_host;
	GtkTreeModel *model;
	GtkEntryCompletion *completion;

	g_return_val_if_fail (builder != NULL, NULL);

	lookup = g_new0 (Netinfo, 1);

	lookup->main_window = GTK_WIDGET (gtk_builder_get_object (builder, "main_window"));
	lookup->progress_bar = GTK_WIDGET (gtk_builder_get_object (builder, "progress_bar"));
	lookup->page_label = GTK_WIDGET (gtk_builder_get_object (builder, "lookup"));
	lookup->running = FALSE;
	lookup->child_pid = 0;
	lookup->host = GTK_WIDGET (gtk_builder_get_object (builder, "lookup_host"));
	lookup->output = GTK_WIDGET (gtk_builder_get_object (builder, "lookup_output"));
	lookup->button = GTK_WIDGET (gtk_builder_get_object (builder, "lookup_button"));
	lookup->type = GTK_WIDGET (gtk_builder_get_object (builder, "lookup_type"));
	lookup->count = NULL;
	lookup->limited = NULL;
	lookup->sensitive = lookup->host;
	lookup->label_run = _("Lookup");
	lookup->label_stop = NULL;
	lookup->routing = NULL;
	lookup->protocol = NULL;
	lookup->multicast = NULL;

	lookup->status_bar = GTK_WIDGET (gtk_builder_get_object (builder, "statusbar"));
	lookup->stbar_text = NULL;
	
	vbox_lookup = GTK_WIDGET (gtk_builder_get_object (builder, "vbox_lookup"));

	label = GTK_WIDGET (gtk_builder_get_object (builder, "lookup_host_label"));
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), lookup->host);
	label = GTK_WIDGET (gtk_builder_get_object (builder, "lookup_type_label"));
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), lookup->type);

	lookup->button_callback = G_CALLBACK (on_lookup_activate);
	lookup->process_line = NETINFO_FOREACH_FUNC (lookup_foreach_with_tree);
	lookup->copy_output = NETINFO_COPY_FUNC (lookup_copy_to_clipboard);

	nettool_lookup_setup_combo_type (lookup);

	model = GTK_TREE_MODEL (gtk_list_store_new (1, G_TYPE_STRING));
	gtk_combo_box_set_model (GTK_COMBO_BOX (lookup->host), model);
	g_object_unref (model);

	/*gtk_combo_box_entry_set_text_column (GTK_COMBO_BOX_ENTRY (lookup->host), 0);*/

	entry_host = GTK_ENTRY (gtk_bin_get_child (GTK_BIN (lookup->host)));

	completion = gtk_entry_completion_new ();
	gtk_entry_set_completion (entry_host, completion);
	g_object_unref (completion);
	gtk_entry_completion_set_model (completion, model);
	gtk_entry_completion_set_text_column (completion, 0);
	g_object_unref (model);

	lookup->history = gn_combo_history_new ();
	gn_combo_history_set_id (lookup->history, "MATE_Network_netinfo_host");
	gn_combo_history_set_combo (lookup->history, GTK_COMBO_BOX (lookup->host));

	g_signal_connect (G_OBJECT (entry_host), "activate",
			  G_CALLBACK (on_lookup_activate),
			  lookup);
	g_signal_connect (G_OBJECT (lookup->button), "clicked",
			  lookup->button_callback,
			  lookup);

	return lookup;
}

/* The value returned must be released from memory */
Netinfo *
load_finger_widgets_from_builder (GtkBuilder * builder)
{
	Netinfo *finger;
	GtkWidget *vbox_finger;
	GtkWidget *label;
	PangoFontDescription *font_desc;
	GtkEntry  *entry_host;
	GtkTreeModel *model;
	GtkEntryCompletion *completion;

	g_return_val_if_fail (builder != NULL, NULL);

	finger = g_new0 (Netinfo, 1);

	finger->main_window = GTK_WIDGET (gtk_builder_get_object (builder, "main_window"));
	finger->progress_bar = GTK_WIDGET (gtk_builder_get_object (builder, "progress_bar"));
	finger->page_label = GTK_WIDGET (gtk_builder_get_object (builder, "finger"));
	finger->running = FALSE;
	finger->child_pid = 0;
	finger->user = GTK_WIDGET (gtk_builder_get_object (builder, "finger_user"));
	finger->host = GTK_WIDGET (gtk_builder_get_object (builder, "finger_host"));
	finger->output = GTK_WIDGET (gtk_builder_get_object (builder, "finger_output"));
	finger->button = GTK_WIDGET (gtk_builder_get_object (builder, "finger_button"));
	finger->type = GTK_WIDGET (gtk_builder_get_object (builder, "finger_type"));
	finger->count = NULL;
	finger->limited = NULL;
	finger->sensitive = GTK_WIDGET (gtk_builder_get_object (builder, "finger_input_box"));
	finger->label_run = _("Finger");
	finger->label_stop = NULL;
	finger->routing = NULL;
	finger->protocol = NULL;
	finger->multicast = NULL;

	finger->status_bar = GTK_WIDGET (gtk_builder_get_object (builder, "statusbar"));
	finger->stbar_text = NULL;
	
	vbox_finger = GTK_WIDGET (gtk_builder_get_object (builder, "vbox_finger"));

	label = GTK_WIDGET (gtk_builder_get_object (builder, "finger_user_label"));
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), finger->user);
	label = GTK_WIDGET (gtk_builder_get_object (builder, "finger_host_label"));
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), finger->host);

	font_desc = pango_font_description_new ();
	pango_font_description_set_family (font_desc, "monospace");
	gtk_widget_modify_font (finger->output, font_desc);
	pango_font_description_free (font_desc);

	finger->button_callback = G_CALLBACK (on_finger_activate);
	finger->process_line = NETINFO_FOREACH_FUNC (finger_foreach);
	finger->copy_output = NETINFO_COPY_FUNC (finger_copy_to_clipboard);

	model = GTK_TREE_MODEL (gtk_list_store_new (1, G_TYPE_STRING));
	gtk_combo_box_set_model (GTK_COMBO_BOX (finger->user), model);
	g_object_unref (model);

	/*gtk_combo_box_entry_set_text_column (GTK_COMBO_BOX_ENTRY (finger->user), 0);*/

	entry_host = GTK_ENTRY (gtk_bin_get_child (GTK_BIN (finger->user)));

	completion = gtk_entry_completion_new ();
	gtk_entry_set_completion (entry_host, completion);
	g_object_unref (completion);
	gtk_entry_completion_set_model (completion, model);
	gtk_entry_completion_set_text_column (completion, 0);
	g_object_unref (model);

	finger->history_user = gn_combo_history_new ();
	gn_combo_history_set_id (finger->history_user, "MATE_Network_netinfo_user");
	gn_combo_history_set_combo (finger->history_user, GTK_COMBO_BOX (finger->user));

	g_signal_connect (G_OBJECT (entry_host), "activate",
			  G_CALLBACK (on_finger_activate),
			  finger);

	model = GTK_TREE_MODEL (gtk_list_store_new (1, G_TYPE_STRING));
	gtk_combo_box_set_model (GTK_COMBO_BOX (finger->host), model);
	g_object_unref (model);

	/*gtk_combo_box_entry_set_text_column (GTK_COMBO_BOX_ENTRY (finger->host), 0);*/

	entry_host = GTK_ENTRY (gtk_bin_get_child (GTK_BIN (finger->host)));

	completion = gtk_entry_completion_new ();
	gtk_entry_set_completion (entry_host, completion);
	g_object_unref (completion);
	gtk_entry_completion_set_model (completion, model);
	gtk_entry_completion_set_text_column (completion, 0);
	g_object_unref (model);

	finger->history = gn_combo_history_new ();
	gn_combo_history_set_id (finger->history, "MATE_Network_netinfo_host");
	gn_combo_history_set_combo (finger->history, GTK_COMBO_BOX (finger->host));

	g_signal_connect (G_OBJECT (entry_host), "activate",
			  G_CALLBACK (on_finger_activate),
			  finger);

	g_signal_connect (G_OBJECT (finger->button), "clicked",
			  finger->button_callback,
			  finger);

	return finger;
}

/* The value returned must be released from memory */
Netinfo *
load_whois_widgets_from_builder (GtkBuilder * builder)
{
	Netinfo *whois;
	GtkWidget *vbox_whois;
	GtkWidget *label;
	GtkEntry  *entry_host;
	GtkTreeModel *model;
	GtkEntryCompletion *completion;
	PangoFontDescription *font_desc;
	

	g_return_val_if_fail (builder != NULL, NULL);

	whois = g_new0 (Netinfo, 1);

	whois->main_window = GTK_WIDGET (gtk_builder_get_object (builder, "main_window"));
	whois->progress_bar = GTK_WIDGET (gtk_builder_get_object (builder, "progress_bar"));
	whois->page_label = GTK_WIDGET (gtk_builder_get_object (builder, "whois"));
	whois->running = FALSE;
	whois->child_pid = 0;
	whois->host = GTK_WIDGET (gtk_builder_get_object (builder, "whois_host"));
	whois->output = GTK_WIDGET (gtk_builder_get_object (builder, "whois_output"));
	whois->button = GTK_WIDGET (gtk_builder_get_object (builder, "whois_button"));
	whois->count = NULL;
	whois->limited = NULL;
	whois->sensitive = GTK_WIDGET (gtk_builder_get_object (builder, "whois_input_box"));
	whois->label_run = _("Whois");
	whois->label_stop = NULL;
	whois->routing = NULL;
	whois->protocol = NULL;
	whois->multicast = NULL;

	whois->status_bar = GTK_WIDGET (gtk_builder_get_object (builder, "statusbar"));
	whois->stbar_text = NULL;

	vbox_whois = GTK_WIDGET (gtk_builder_get_object (builder, "vbox_whois"));

	label = GTK_WIDGET (gtk_builder_get_object (builder, "whois_host_label"));
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), whois->host);

	font_desc = pango_font_description_new ();
	pango_font_description_set_family (font_desc, "monospace");
	gtk_widget_modify_font (whois->output, font_desc);
	pango_font_description_free (font_desc);

	whois->button_callback = G_CALLBACK (on_whois_activate);
	whois->process_line = NETINFO_FOREACH_FUNC (whois_foreach);
	whois->copy_output = NETINFO_COPY_FUNC (whois_copy_to_clipboard);

	model = GTK_TREE_MODEL (gtk_list_store_new (1, G_TYPE_STRING));
	gtk_combo_box_set_model (GTK_COMBO_BOX (whois->host), model);
	g_object_unref (model);

	/*gtk_combo_box_entry_set_text_column (GTK_COMBO_BOX_ENTRY (whois->host), 0);*/
	
	entry_host = GTK_ENTRY (gtk_bin_get_child (GTK_BIN (whois->host)));
	
	completion = gtk_entry_completion_new ();
	gtk_entry_set_completion (entry_host, completion);
	g_object_unref (completion);
	gtk_entry_completion_set_model (completion, model);
	gtk_entry_completion_set_text_column (completion, 0);
	g_object_unref (model);

	whois->history = gn_combo_history_new ();
	gn_combo_history_set_id (whois->history, "MATE_Network_netinfo_domain");
	gn_combo_history_set_combo (whois->history, GTK_COMBO_BOX (whois->host));

	g_signal_connect (G_OBJECT (entry_host), "activate",
			  G_CALLBACK (on_whois_activate),
			  whois);
	g_signal_connect (G_OBJECT (whois->button), "clicked",
			  whois->button_callback,
			  whois);

	return whois;
}
