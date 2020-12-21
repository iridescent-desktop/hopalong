# Hopalong

Hopalong is a simple Wayland compositor with a featureset comparable to XFWM.

## Design Goals

* Window managers should stay out of the user's way and be as boring as possible.
  * No wobbly windows or other complexity.
  * Simple chrome with color themes: Choose a primary color and we do the rest.
  * Designed for people who like GNU nano as a primary editor.

* Minimal resource utilization.

* Correct functionality regardless of system byte ordering (endianness).

* Built on wlroots: a rock-solid Wayland compositor framework powering other
  compositors such as Sway.

## Screenshots

TODO: First, screenshot support must be implemented.

## Install

TODO: Document how to install this crime against humanity.

## TODO

* XWayland shell
* Damage (partial rendering of the desktop)
