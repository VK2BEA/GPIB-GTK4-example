# GPIB GTK4 example
This program demonstrates a way of integrating GPIB communication into
a GTK4 GUI application without explicitly creating additional threads.
This has the advantage of being able to directly interact with GUI widgets
without having to synchronize to the main loop.

The example is written for an HP8565E Spectrum analyzer instrument, so you will have to modify
it to work with your instrument. (most likely you will need to change the "`ID?`" to the
command for your instrument (say "`*IDN?`") and the function to demonstrate the
SRQ capability which is highly instrument dependent. Make the changes in the `instrument.c` file.

A `GSource` is created when opening the GPIB device. This is attached to the [main loop](https://docs.gtk.org/glib/main-loop.html)
and checks the progress of asynchronous read and writes on the GPIB. The assigned
callback associated with the GSource will complete the GPIB action when the
condition is met.

Two simple examples are shown:

`GPIB_device_ID_example()`
1. Send the "`ID?`" query to the HP8565E on the GPIB
2. Read the response from the HP8565E
3. Update the GUI GtkLabel widget with the ID string.

`GPIB_illegal_cmd_SRQ_example()`
1. Send an illegal command (which will trigger an SRQ) to the HP8565E on the GPIB
2. Wait on an SRQ from the HP8565E
3. Handle the SRQ when received

You require the GTK4 development libraries and Linux GPIB user libraries installed.
[`https://linux-gpib.sourceforge.io/`](https://linux-gpib.sourceforge.io/)

Compile with:
```
gcc -o GPIB-GTK4-demo `pkg-config --cflags --libs gtk4` -lgpib GPIB_GTK4-demo.c GPIB_source.c  instrument.c  utility.c
```

The program options are:
```
  -d, --GPIBdeviceID            GPIB device ID for HPGL plotter
  -c, --GPIBcontrollerIndex     GPIB controller board index
  -e, --EOIonLF                 End GPIB read on LF character
```
