# ble-midi2usbhost
A Pico W Bluetooth LE MIDI adapter for any class compliant Bluetooth MIDI device

This project uses the native Raspberry Pi Pico W USB port as a USB MIDI host port
and it uses the Bluetooth LE capability of the Pico W's CYW43 WiFi/Bluetooth module
to create a Bluetooth LE MIDI GATT Server. You can power the Raspberry Pi Pico W
board with any 5V USB C power supply, including 5V phone charger battery packs.

This project uses "Just Works" pairing and no
bonding to reduce the likelyhood of randomly connecting to the wrong device.

This is both a hardware and software project. Please test carefully any hardware you
build before you connect your expensive MIDI device to this project. Also be aware that
the only current limiting or short circuit protection this project provides for its
USB host port comes from the external 5V power source.

# Hardware
I do not get money from hardware vendors. The links to hardware vendors are for illustration
and are not endorsements.

## Bill of Materials
1. A Pico W board see the [Raspberry Pi website](https://www.raspberrypi.com/documentation/microcontrollers/raspberry-pi-pico.html) for more info
2. A USB C breakout board (e.g. [Adafruit 4090](https://www.adafruit.com/product/4090))
3. A microUSB OTG to USA A adapter (e.g. [Adafruit 1099](https://www.adafruit.com/product/1099))
4. Wire and solder
5. 5V USB C power supply (e.g., a cell phone charger or cell phone battery pack) and USB C charging cable
6. 3-pin 0.1" spacing square pin header for connecting the picoprobe debug pins and ground (optional)
7. 2-pin 0.1" spacing square pin header for connecting the picoprobe ground pins (optional)
8. A Pico board board programmed as a picoprobe and populated with headers for wiring to the Pico W board (optional)
9. 5 Jumper wires for connecting the picoprobe to the Pico W (optional)
10. A microUSB cable to connect the picoprobe to your debug host computer

## Wiring
1. Solder a wire between pin 40 (VBUS pin) of the Pico W and the VBUS pin of the USB C breakout board
2. Solder a wire between pin 38 (GND pin) of the Pico W and the GND pin of the USB C breakout board
3. Plug a USB C power cable to the USB C breakout board (but do not activate the 5V power yet).
4. If you are using a second Pico board programmed picoprobe  for debug and serial port
   1. Solder the 2-pin header to pins 1 and 2 of the Pico W board
   2. Solder the 3-pin header to the 3 debug pins of the Pico W board
   3. Attach the microUSB cable to the picoprobe, but do not attach it to the host computer yet.
   4. Use jumper wires to connect the picoprobe to the Pico W board per the instructions in Appendix A of the
[Getting Started with Raspberry Pi Pico](https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf) document

# Software
## Getting the software
## Build the Project
