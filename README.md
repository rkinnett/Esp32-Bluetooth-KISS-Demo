# Esp32 Bluetooth KISS Demo
Demonstration of [KISS protocol](https://en.wikipedia.org/wiki/KISS_(TNC)) running on Bluetooth serial.

More generally, this demo abstracts the KISS interface to be assigned to *any* Stream object!

This demo implements the KISS protocol only.  
It does does not implement AFSK modulation/demodulation.

The default configuration assigns the Bluetooth serial stream to the KISS interface, and assigns the (USB) Serial device as the target for log messages.  These assignments are easy changed within the Setup function of the main arduino sketch, and can be reconfigured on the fly.
