/*
 * GPIB_source.c
 *
 * © Copyright 2026 Michael G. Katzmann
 * SPDX-License-Identifier: Apache-2.0
**/

/* See https://docs.gtk.org/glib/main-loop.html
 * The Main Event Loop
 *
 * The main event loop manages all the available sources of events for GLib and GTK applications.
 * These events can come from any number of different types of sources such as file descriptors
 * (plain files, pipes or sockets) and timeouts. New types of event sources can also be added
 * using g_source_attach().
 *
 * This is what we do to add the GPIB device to the main loop.
**/
#include <stdio.h>
#include <stdlib.h>

#include <gtk/gtk.h>
#include <glib.h>
#include <gpib/ib.h>

#include "GPIB_GTK4-demo.h"



/**
 * PREPARE: Tells the main loop how long it can sleep before checking this source.
 * We set a 10ms timeout if we are waiting for GPIB response so the main loop
 * doesn't consume 100% CPU while waiting for the hardware DMA/interrupts to finish.
 */
gboolean GPIBsourcePrepare(GSource *source, gint *timeout) {
	tGPIBsource *pGPIBsource = (tGPIBsource *)source;
	if( pGPIBsource->eGPIBstate != G_IDLE )
        *timeout = 10; // Sleep up to 10 milliseconds
	else
		*timeout = -1;

    return FALSE;  // Return FALSE because we aren't definitively ready yet
}


/**
 * CHECK: Called after the main loop wakes up. We do our (almost) non-blocking hardware poll here.
 */
gboolean GPIBsourceCheck(GSource *source) {
    tGPIBsource *pGPIBsource = (tGPIBsource*) source;
    gint waitStatus;

    // Are we not doing anything?
    // Nothing to check if we are idle.
    if (pGPIBsource->eGPIBstate == G_IDLE)
        return FALSE;

    // We do our own timeout. ibtmo() has been set to TNONE
    if (pGPIBsource->timeOut > 0.0 &&
    		g_timer_elapsed(pGPIBsource->timerElapsed, NULL) > pGPIBsource->timeOut) {
        g_printerr("GPIB timeout\n");
        ibstop(pGPIBsource->GPIBdevice);
        pGPIBsource->flags.bTimeout = true;
        // dispatch will run the callback
        return TRUE;
    } else {
        pGPIBsource->flags.bTimeout = false;
    }

    // Get the updated status but do not block (wait 0).
    // We will only dispatch if a bit in the status mask matches or there is an error
    waitStatus = ibwait(pGPIBsource->GPIBdevice, 0x000);
    if( waitStatus & (pGPIBsource->ibstaMask | ERR) ) {
        if (waitStatus & ERR) {
            ibstop(pGPIBsource->GPIBdevice);
        }
        if ((waitStatus & CMPL) != 0) {
            // ibwait must be called with CMPL to end the read/write
            // but this will return immediately because we already know CMPL is set
            ibwait(pGPIBsource->GPIBdevice, CMPL);
        }
        return TRUE;	// Dispatch and run callback
    }

    return FALSE; // Still waiting on the hardware...
}

/**
 * DISPATCH: If check returned TRUE, this fires the user's callback.
 */
gboolean GPIBsourceDispatch(GSource *source, GSourceFunc callback, gpointer gpGlobalData) {
    __attribute__((unused))  tGPIBsource *pGPIBsource = (tGPIBsource*) source;
    GPIBsourceFunc GPIBcallback = (GPIBsourceFunc) callback;

    if (!GPIBcallback) {
        return G_SOURCE_REMOVE;
    }
    // Fire the application's callback, passing the device and the status
    return GPIBcallback(source, gpGlobalData);
}

/**
    Called when the source is finalized. At this point, the source will have been destroyed,
    had its callback cleared, and have been removed from its GMainContext, but it will still
    have its final reference count, so methods can be called on it from within this function.
 */
void GPIBsourceFinalize(GSource *source) {
    __attribute__((unused))  tGPIBsource *pGPIBsource = (tGPIBsource*) source;
}

// Map our custom functions into the GLib struct
// See https://docs.gtk.org/glib/struct.SourceFuncs.html
GSourceFuncs GPIBsourceFunctions = {
        GPIBsourcePrepare,
        GPIBsourceCheck,
        GPIBsourceDispatch,
		GPIBsourceFinalize
};

// Create the augmented GSource
// The GSource is the first item in the structure and additional
// variables are following (in our case a pointer to the tGPIBcontext structure)
// wich we initialize
GSource* GPIBsourceNew( gint GPIBdevice ) {
    GSource *source = g_source_new(&GPIBsourceFunctions, sizeof(tGPIBsource));
    tGPIBsource *pGPIBsource = (tGPIBsource*) source;

    pGPIBsource->GPIBdevice = GPIBdevice;
    pGPIBsource->eGPIBstate = G_IDLE;

    // Create a timer... dispose of when the source is destroyed
    pGPIBsource->timerElapsed = g_timer_new();
    pGPIBsource->timeOut = 5.0;

    return source;
}

// open the GPIB device, create GSource for the GPIB I/O
// and attach the GSource to the main loop
tGPIBsource *openGPIBdevice( gint GPIBcontrollerIdx, gint GPIBdevicePID, gboolean bEOIonEOS ) {

    GSource *pGPIBsource;

    // GPIB
    gint GPIBdevice;

    // Open the device (Minor 11 / PAD '4' to match your instrument)
    GPIBdevice = ibdev( GPIBcontrollerIdx, GPIBdevicePID, 0, T10s, bEOIonEOS, 0x0A);

    if (GPIBdevice < 0) {
        g_printerr("Failed to open device  (controller %d / device PID %d).\n", GPIBcontrollerIdx, GPIBdevicePID);
        return NULL;
    }

    ibtmo(GPIBdevice, TNONE);
    // for SRQ/RQS
    ibconfig( GPIBcontrollerIdx, IbcAUTOPOLL, true );

    // Create and attach our GPIB GSource
    pGPIBsource = GPIBsourceNew( GPIBdevice );

    g_source_set_name(pGPIBsource, "GPIB");     // not required, but useful for debugging

    g_source_attach(pGPIBsource, NULL);
    g_source_unref(pGPIBsource);                // Main loop takes ownership

    return (tGPIBsource *)pGPIBsource;
}



