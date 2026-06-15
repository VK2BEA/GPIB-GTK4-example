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

    gint GPIBdevicePID;     // The GPIB device descriptor
    gint GPIBcontrollerIdx; //

    struct {
        guint32 bEOIonLF :1;
    } flags;

    enum { W_LBL_NOTE, W_BTN_ID, W_BTN_SRQ, W_LAST } eWidget;
    GtkWidget *widgets[ W_LAST ];
    GtkApplication *app;

} tGlobalData;

extern tGlobalData globalData;

// GSource implementation

// The internal state of our custom GSource
typedef struct {
    GSource source;     // the opaque GSource structure
    // Additional data available to the prepare/check/dispatch/finalize routines
    // the current GPIB function
    enum {
        G_IDLE,
        G_01, G_02, G_03, G_04, G_05, G_06, G_07, G_08, G_09, G_10
    } eGPIBstate;

    gint GPIBdevice;     // The GPIB device descriptor

    gint ibstaMask;     // the bits in the GPIB status word to wait on

    struct {
        guint32 bTimeout :1;
        guint32 bEOIonLF :1;
    } flags;

    GTimer *timerElapsed;
    gdouble timeOut;

    gchar *inputBuffer;

    tGlobalData *pGlobalData;
} tGPIBsource;

// The function signature for our application callbacks
typedef gboolean (*GPIBsourceFunc)(GSource *, gpointer);

void gprint_hex(const gpointer, gsize);

tGPIBsource *openGPIBdevice( gint controllerID, gint deviceID, gboolean bEOIonEOS );

extern GSourceFuncs GPIBsourceFunctions;
gboolean GPIBsourcePrepare(GSource *, gint *);
gboolean GPIBsourceDispatch(GSource *, GSourceFunc, gpointer);
gboolean GPIBsourceCheck(GSource *);

gboolean GPIB_device_ID_example(GSource *, gpointer);
gboolean GPIB_illegal_cmd_SRQ_example(GSource *, gpointer);

void notify(tGlobalData *, gchar *);

#define ERROR (-1)
#define INVALID (-1)

#endif /* GPIB_GTK4_DEMO_H_ */
