/*
 * instrument.c
 *
 * © Copyright 2026 Michael G. Katzmann
 * SPDX-License-Identifier: Apache-2.0
**/

#include <stdio.h>
#include <stdlib.h>

#include <gtk/gtk.h>
#include <glib.h>
#include <gpib/ib.h>

#include "GPIB_GTK4-demo.h"

/**
 *
 * Initially send the string "ID?" to the device (HP8595E).
 * This function is then called again by the GSource dispatch function when
 * the defined condition of ibstat is met. (CMPL or ERR)
 *
 * The series of events are executed to achieve the required function.
 * In this example, it is quite simple:
 * 1. send "ID?" with an asynchronous write (ibwrta)
 * 2. read the reply with an asynchronous read (ibrda)
 * 3. at EOI or EOS, receive ID string and display it
 *
 */
gboolean GPIB_device_ID_example(GSource *source, gpointer gpGlobalData) {

    tGlobalData *pGlobalData = (tGlobalData *)gpGlobalData;
    tGPIBsource *pGPIBsource = (tGPIBsource *)source;

    gint GPIBdevice = pGPIBsource->GPIBdevice;
    gint status = 0;
    gint count = 0;

    __attribute__((unused)) gboolean bComplete = (status & CMPL);

    const gchar *sQuery = "ID?;";

    // Error
    if( pGPIBsource->eGPIBstate != G_IDLE ) {
	    status = AsyncIbsta();
	    count = AsyncIbcnt();
    	// Check for error & act appropriately
		if (status & ERR) {
			g_printerr("GPIB Error encountered! (%s)\n", gpib_error_string(iberr));
			notify( pGlobalData, (gchar *)gpib_error_string(iberr));
			pGPIBsource->eGPIBstate = G_IDLE;
			return G_SOURCE_CONTINUE;
		}

		// Check for timeout & act appropriately
		if (pGPIBsource->flags.bTimeout) {
		    notify( pGlobalData,  "HP8585A did not respond (timeout)");
			pGPIBsource->eGPIBstate = G_IDLE;
			return G_SOURCE_CONTINUE;
		}
    }

    // State machine for instrument action
    switch (pGPIBsource->eGPIBstate) {
    case G_IDLE:
    	// initially this is called by the user program to initiate the sequence
    	// we send the "ID?" string to the instrument.

        // Set the callback for the GSource dispatch function to handle the sequence of events:
        // completion of write, the reading of the ID string and display.
        g_source_set_callback(source, (GSourceFunc) GPIB_device_ID_example, pGlobalData, NULL);
        ibwrta(pGPIBsource->GPIBdevice, sQuery, strlen(sQuery));  // issue the ID? query
        pGPIBsource->ibstaMask = CMPL;				// we wait for either the completion of the write or an error
        pGPIBsource->timeOut = 2.0;        		// 2 second timeout
        pGPIBsource->eGPIBstate = G_01;			// next state of the state machine
    	break;
    case G_01:
        g_printerr( "Async write complete (%d chars). Initiating async read...\n",
                count);
        // allocate a buffer for the ID string to hold the data read from the instrument
        pGPIBsource->inputBuffer = g_realloc( pGPIBsource->inputBuffer, RD_BUF_SIZE );
        ibrda(GPIBdevice, pGPIBsource->inputBuffer, RD_BUF_SIZE - 1);
        pGPIBsource->eGPIBstate = G_02;      		// the next state of the sequence
        break;
    case G_02:
        g_printerr( "Async read complete (0x%04x) (%d chars).\n", status, count );
        // Null-terminate the string (being careful not to write out of bounds)
        pGPIBsource->inputBuffer[ count % RD_BUF_SIZE ] = '\0';
        // some debugging ... show hex on terminal
        gprint_hex(pGPIBsource->inputBuffer, count);
        // Show the ID string returned from instrument on the GUI
        notify( pGlobalData,  pGPIBsource->inputBuffer);
        pGPIBsource->eGPIBstate = G_IDLE;			// GSource check function will not look at the GPIB device
        break;

    default:
        pGPIBsource->eGPIBstate = G_IDLE;
        break;
    }

    // We are now waiting on a GPIB asynchronous read, write or SRQ (or have completed the action sequence).
    // This function will be called again by the GSource dispatch function if the GSource check function
    // determines that the condition of ibstat is met.
    // In the mean time the GUI continues processing and is responsive.

    g_timer_start(pGPIBsource->timerElapsed);		// reset timeout timer
    pGPIBsource->flags.bTimeout = false;			// we reset the timeout flag
    return G_SOURCE_CONTINUE;
}

/* N.B. ⚠️ This is specific to the HP8595E Spectrum Analyzer ⚠️
 * SRQ is highly instrument dependent. Modify for any other instrument.
 *
 * Example GPIB action to show how a device SRQ is handled
 * The GPIB controller is configured for AUTOPOLL, so that the
 * controller obtains the status byte from the device asserting the SRQ.
 * N.B. the ibwait() documentation says..
 *     For the RQS bit to be set in the returned ibsta automatic serial polling
 *     must be enabled for the board controlling the device, see ibconfig().
 *     The RQS condition is cleared by serial polling the device, see ibrsp().
 *
 * In this simple demo, the HP8595E asserts the SRQ if it receives an invalid instruction.
 * An invalid instruction "XYZ" is written to the device and we wait the RQS bit
 * to be set in the status.
 *
 *
 * The bits of the HP8595E Status Byte returned by ibrsp() are:
 * Bit 0: Not used / Always 0
 * Bit 1: Units key pressed
 * Bit 2: End of sweep
 * Bit 3: Hardware broken
 * Bit 4: Command complete
 * Bit 5: Illegal command
 * Bit 6: Not used / Always 0
 * Bit 7: Not used / Always 0
 */
gboolean GPIB_illegal_cmd_SRQ_example(GSource *source, gpointer gpGlobalData) {

    tGlobalData *pGlobalData = (tGlobalData *)gpGlobalData;
    tGPIBsource *pGPIBsource = (tGPIBsource *)source;

    gint GPIBdevice = pGPIBsource->GPIBdevice;

    gint status = 0;
    gint count = 0;

    __attribute__((unused)) gboolean bComplete = (status & CMPL);

    const gchar *sIllegalCommand = "XYZ;";
    gchar statusByte, sString[ 20 ];

    // Error (only relevant if we have previously issued a GPIB command)
    if( pGPIBsource->eGPIBstate != G_IDLE ) {
	    status = AsyncIbsta();
	    count = AsyncIbcnt();

		if (ibsta & ERR) {
			g_printerr("GPIB Error encountered! (%s)\n", gpib_error_string(iberr));
			notify( pGlobalData, (gchar *)gpib_error_string(iberr));
			pGPIBsource->eGPIBstate = G_IDLE;
			return G_SOURCE_CONTINUE;
		}

		if (pGPIBsource->flags.bTimeout) {
		    notify( pGlobalData, "HP8585A did not respond (timeout)");
		    pGPIBsource->eGPIBstate = G_IDLE;
			return G_SOURCE_CONTINUE;
		}
    }

    // State machine for instrument action
    switch (pGPIBsource->eGPIBstate) {
    case G_IDLE:
    	// The starting point for the sequence of events
    	// Here, we:
    	// 1. define what bits in the ibsta word we will look for (CMPL)
    	// 2. set the timeout value (2 seconds)
    	// 3. set the callback function, called from the dispatch function for the GPIB source (this function)
    	// 4. send the illegal command to the GPIB device (which will trigger an SRQ)
    	// In the case of the HP8595E, an illegal command will trigger an SRQ. This is unlikely
    	// to be the case for most GPIB devices. You will need to make changes for it to work on other devices.

        // Callback to handle the sequence of events: completion of write and the reading
        // of the ID string (this function).
        g_source_set_callback(source, (GSourceFunc) GPIB_illegal_cmd_SRQ_example, pGlobalData, NULL);
        // issue the ID? query
        ibwrta(GPIBdevice, sIllegalCommand, strlen(sIllegalCommand));
        pGPIBsource->ibstaMask = CMPL;				// we wait for either the completion of the write or an error
        pGPIBsource->timeOut = 2.0;        		// 2 second timeout
        pGPIBsource->eGPIBstate = G_01;			// next state of the state machine
    	break;
    case G_01:
    	// We get here after the previous step if the write completes
    	// Here we:
    	// 1. define what bits in the ibsta word we will look for (RQS)
    	// 2. reset the timeout timer (unchanged 2 second timeout)
    	g_printerr( "Async write complete (%d chars). Expecting SRQ...\n", count);
    	pGPIBsource->ibstaMask = RQS;				// we wait for RQS
    	pGPIBsource->eGPIBstate = G_02;			// next state of the state machine
        break;
    case G_02:
    	g_printerr( "SRQ recognized: ibsta (0x%04x) / ", ibsta );
    	ibrsp(GPIBdevice, &statusByte);
        g_printerr( "Device status byte (0x%02x)\n", statusByte );
        g_snprintf( sString, sizeof(sString), "STB 0x%02x", statusByte);
        notify( pGlobalData, sString);
        pGPIBsource->eGPIBstate = G_IDLE;			// return to idle state
        break;

    default:
        pGPIBsource->eGPIBstate = G_IDLE;
        break;
    }

    // We are now waiting on a GPIB asynchronous read, write or SRQ (or have completed the action sequence).
    // This function will be called again by the GSource dispatch function if the GSource check function
    // determines that the condition of ibstat is met.
    // In the mean time the GUI continues processing and is responsive.

    g_timer_start(pGPIBsource->timerElapsed);		// reset timeout timer
    pGPIBsource->flags.bTimeout = false;			// we reset the timeout flag
    return G_SOURCE_CONTINUE;
}
