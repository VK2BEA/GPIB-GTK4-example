/*
 * GPIB_GTK4.h
 *
 * © Copyright 2026 Michael G. Katzmann
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef GPIB_GTK4_DEMO_H_
#define GPIB_GTK4_DEMO_H_

#define	true    (1)
#define false   (0)

#define RD_BUF_SIZE 1024

typedef struct {
    // the current GPIB function
    enum {
        G_IDLE,
		G_01, G_02, G_03, G_04, G_05, G_06, G_07, G_08, G_09
    } eGPIBstate;

    gint GPIBdevice;     // The GPIB device descriptor
    gint GPIBcontroller;  //

    gint ibstaMask;		// the bits in the GPIB status word to wait on

    struct {
    	guint32 bTimeout :1;
    	guint32 bEOIonLF :1;
    } flags;

    GTimer *timerElapsed;
    gdouble timeOut;

    // data buffer (malloced and freed as required)
    gchar *buffer;
    // other data
    GtkApplication *app;
    GtkLabel *wLabel;
} tGPIBcontext;

extern tGPIBcontext GPIBcontext;

// GSource implimentation

// The internal state of our custom GSource
typedef struct {
    GSource source;     // the opaque GSource structure
    // Additional data available to the prepare/check/dispatch/finalize routines
    tGPIBcontext *pContext;
} tGPIBsource;

// The function signature for our application callbacks
typedef gboolean (*GPIBsourceFunc)(GSource *, gpointer);

void gprint_hex(const gpointer, gsize);

tGPIBsource *openGPIBdevice( tGPIBcontext * );

extern GSourceFuncs GPIBsourceFunctions;
gboolean GPIBsourcePrepare(GSource *, gint *);
gboolean GPIBsourceDispatch(GSource *, GSourceFunc, gpointer);
gboolean GPIBsourceCheck(GSource *);

gboolean GPIB_device_ID_example(GSource *, gpointer);
gboolean GPIB_illegal_cmd_SRQ_example(GSource *, gpointer);


#define ERROR (-1)
#define INVALID (-1)

#endif /* GPIB_GTK4_DEMO_H_ */
