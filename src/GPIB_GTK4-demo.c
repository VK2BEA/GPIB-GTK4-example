/*
 * GPIB_GTK4-demo.c
 *
 * © Copyright 2026 Michael G. Katzmann
 * SPDX-License-Identifier: Apache-2.0
 *
 * This program demonstrates a way of integrating GPIB communication into
 * a GTK4 GUI application without explicitly creating additional threads.
 * This has the advantage of being able to directly interacting with GUI widgets
 * without having to synchronize to the main loop.
 *
 * A GSource is created when opening the GPIB device. This is attached to the main loop
 * and checks the progress of asynchronous read and writes on the GPIB. The assigned
 * callback associated with the GSource will complete the GPIB action when the
 * condition is met.
 *
 * Two simple examples are shown:
 * GPIB_device_ID_example()
 *      1. Send the "ID?" query to the HP8565E on the GPIB
 *      2. Read the response from the HP8565E
 *      3. Update the GUI GtkLabel widget with the ID string.
 *
 * GPIB_illegal_cmd_SRQ_example()
 *      1. Send an illegal command (which will trigger an SRQ) to the HP8565E on the GPIB
 *      2. Wait on an SRQ from the HP8565E
 *      3. Handle the SRQ when received
 */

#include <stdio.h>
#include <stdlib.h>

#include <gtk/gtk.h>
#include <glib.h>
#include <gpib/ib.h>

#include "GPIB_GTK4-demo.h"

static gint     optDebug = 0;
static gboolean bEOIonLF = true;
static gint     optDeviceID = 4;
static gint     optControllerIndex = 0;
static gchar    **argsRemainder = NULL;

static const GOptionEntry optionEntries[] =
{
        { "debug",                    'b', G_OPTION_FLAG_NONE, G_OPTION_ARG_INT,
            &optDebug, "Print diagnostic messages in journal (0-7)", NULL },
        { "GPIBdeviceID",             'd', G_OPTION_FLAG_NONE, G_OPTION_ARG_INT,
            &optDeviceID, "GPIB device ID for HPGL plotter", NULL },
        { "GPIBcontrollerIndex",      'c', G_OPTION_FLAG_NONE, G_OPTION_ARG_INT,
            &optControllerIndex, "GPIB controller board index", NULL },
        { "EOIonLF",                  'e', G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE,
            &bEOIonLF, "End GPIB read on LF character", NULL },
        { G_OPTION_REMAINING, 0, G_OPTION_FLAG_NONE, G_OPTION_ARG_STRING_ARRAY, &argsRemainder, "", NULL },
        { NULL }
};

// Callback function triggered when the "ID?" button is clicked
static void on_btn_click_ID(GtkWidget *button, gpointer udata) {
    // Cast the uData back to a GtkWidget (the label)
    tGPIBsource *pGPIBsource = (tGPIBsource*) udata;
    tGPIBcontext *pContext = pGPIBsource->pContext;

    // Ignore button if we are busy
    if (pContext->eGPIBstate != G_IDLE)
        return;

    gtk_label_set_text(GTK_LABEL(pContext->wLabel), "Waiting for ID string");
    GPIB_device_ID_example((GSource *)pGPIBsource, pContext);

}

// Callback function triggered when the "SRQ" button is clicked
static void on_btn_click_SRQ(GtkWidget *button, gpointer udata) {
    // Cast the uData back to a GtkWidget (the label)
    tGPIBsource *pGPIBsource = (tGPIBsource*) udata;
    tGPIBcontext *pContext = pGPIBsource->pContext;

    // Ignore button if we are busy
    if (pContext->eGPIBstate != G_IDLE)
        return;

    gtk_label_set_text(GTK_LABEL(pContext->wLabel), "Waiting for SRQ");
    GPIB_illegal_cmd_SRQ_example((GSource *)pGPIBsource, pContext);

}

/* on_shutdown (shutdown signal callback)
 *
 * Cleanup on shutdown
 *
 */
static void on_shutdown (GApplication *app, gpointer udata)
{
    tGPIBcontext *pContext = (tGPIBcontext*) udata;
    g_timer_destroy(pContext->timerElapsed);
    g_free( pContext->buffer ); // we malloced this structure, so now dispose of it
    ibonl(pContext->GPIBdevice, true);
}




// Function triggered when the application is activated
static void on_activate(GtkApplication *app, gpointer udata) {
    GtkWidget *wWindow_Application;
    GtkWidget *wBox;
    GtkWidget *wButton_ID, *wButton_SRQ;
    GtkWidget *wLabel_Notification;

    tGPIBsource *pGPIBsource;

    // Create the GUI.
    // Here we create the GUI discretely but it is just for simplicity.
    // A better method is to use the GUI builder which processes compiled or plain XML resource file
    // descriptions of the GUI.
    // I recommend the WYSIWYG "Cambalache" tool to create the GUI.

    // Create the main application wWindow_Application
    wWindow_Application = gtk_application_window_new(app);
    gtk_window_set_default_size(GTK_WINDOW(wWindow_Application), 300, 200);

    // 2. Create a vertical box container to hold multiple widgets
    // The '10' is the spacing (in pixels) between children
    wBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_valign(wBox, GTK_ALIGN_CENTER);
    gtk_widget_set_halign(wBox, GTK_ALIGN_CENTER);

    // Add the box to the wWindow_Application
    gtk_window_set_child(GTK_WINDOW(wWindow_Application), wBox);
    // Create the label widget
    wLabel_Notification = gtk_label_new("⚠️ Modify program for your Instrument");
    // just one line
    gtk_label_set_single_line_mode ( GTK_LABEL(wLabel_Notification), true );
    // Add the label to the vertical wBox
    gtk_box_append(GTK_BOX(wBox), wLabel_Notification);

    // Create the button widgets
    wButton_ID = gtk_button_new_with_label("Send ID?");
    wButton_SRQ = gtk_button_new_with_label("Illegal Code SRQ");
    // Add the buttons to the vertical wBox
    gtk_box_append(GTK_BOX(wBox), wButton_ID);
    gtk_box_append(GTK_BOX(wBox), wButton_SRQ);


    // Open the GPIB device and create the GSource to handle the GPIB device
    // within the glib main loop. https://docs.gtk.org/glib/main-loop.html
    if( (pGPIBsource = openGPIBdevice( &GPIBcontext )) == NULL ) {
        g_application_quit(G_APPLICATION(app));
    }

    gchar *sTitle = g_strdup_printf( "🛈 GPIB: %d / PID %d", GPIBcontext.GPIBcontroller, GPIBcontext.GPIBdevice );
    gtk_window_set_title(GTK_WINDOW(wWindow_Application), sTitle);
    g_free( sTitle );

    // Keep a note of the label widget so we can update the text
    pGPIBsource->pContext->wLabel = GTK_LABEL(wLabel_Notification);
    pGPIBsource->pContext->app = app;

    // Connect the button's "clicked" signal to our callback functions
    g_signal_connect(wButton_ID, "clicked", G_CALLBACK(on_btn_click_ID), pGPIBsource);
    g_signal_connect(wButton_SRQ, "clicked", G_CALLBACK(on_btn_click_SRQ), pGPIBsource);

    // 6. Display the window and all its child widgets
    gtk_window_present(GTK_WINDOW(wWindow_Application));
}

static void
on_startup (GApplication *app, gpointer udata)
{
    tGPIBcontext *pContext = (tGPIBcontext*) udata;

    pContext->GPIBdevice = optDeviceID;
    pContext->GPIBcontroller = optControllerIndex;
    pContext->flags.bEOIonLF = bEOIonLF;
}

int main(int argc, char **argv) {
    GtkApplication *app;
    int status;

    // Create a new GTK application instance
    app = gtk_application_new("us.heterodyne.GPIBexample", G_APPLICATION_DEFAULT_FLAGS);
    g_application_add_main_option_entries (G_APPLICATION ( app ), optionEntries);

    // Connect the "activate" signal to set up the UI
    g_signal_connect(app, "startup",  G_CALLBACK  (on_startup), (gpointer)&GPIBcontext);
    g_signal_connect(app, "activate", G_CALLBACK(on_activate), (gpointer)&GPIBcontext);
    g_signal_connect(app, "shutdown", G_CALLBACK (on_shutdown), (gpointer)&GPIBcontext);


    // Run the application (this blocks until the application exits)
    status = g_application_run(G_APPLICATION(app), argc, argv);

    // Clean up memory
    g_object_unref(app);

    return status;
}
