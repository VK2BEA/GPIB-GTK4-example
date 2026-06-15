/*
 * utility.c
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

void gprint_hex(const gpointer binary_data, gsize data_length) {

    for (gsize i = 0; i < data_length; i++) {
        // Format each byte into exactly 2 hex digits
        g_printerr("%02x ", ((guint8*) binary_data)[i]);
    }
    g_printerr("\n");
}
