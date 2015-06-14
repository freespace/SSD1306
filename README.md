# Introduction
This is an ad-hoc I2C library for my [xadow-watch](https://github.com/freespace/xadow-watch) project.
It was written mostly as an excercise in interfacing with a display driver, and to move the xadow-watch
project forward.

I have made *no* attempt to make the code generic, and in the interest of saving RAM it uses a resizable
buffer to update the display. This mechanism may be of some interest as the amount of RAM used for display
can be customised.

# Buffer mechanism

In essence, buffer in RAM is mapped to display via an origin and a size. When drawing to the buffer has
finished, the buffer is uploaded to display memory. The size of the buffer is fixed, but its *dimensions*
are configurable at runtime, as is the origin. This allows, in theory, a single-byte buffer of 1x8 pixels.
