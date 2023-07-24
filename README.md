# ble-midi2usbhost
A Pico W Bluetooth LE MIDI to USB Host adapter for any class compliant USB MIDI device

This project uses the native Raspberry Pi Pico W USB port as a USB MIDI host port
and it uses the Bluetooth LE capability of the Pico W's CYW43 WiFi/Bluetooth module
to create a Bluetooth LE MIDI GATT Server. You can power the Raspberry Pi Pico W
board with any 5V USB C power supply. If you choose to use a 5V phone charger
battery pack, make sure that the attached USB MIDI device is bus powered and
uses enough power to keep the phone charger battery pack from shutting off
automatically.

Because some BLE-MIDI controller apps use a fixed, 0-value timestamp for every message,
this code does not attempt to synchronize timestamps to the clock.

This is both a hardware and software project. Please test carefully any hardware you
build before you connect your expensive MIDI device to this project. Also be aware that
the only current limiting or short circuit protection this project provides for its
USB host port comes from the external 5V power source.

If you have problems or need, please file issues in the Issues tab on the project
[home page](https://github.com/rppicomidi/ble-midi2usbhost). If you have specific
Bluetooth issues and can attach a packet capture trace, that can help a lot. If
you find a bug and have a fix for it, pull requests are welcome.

# Known issues
## 24-Jul-2023:
If the Pico W pairs, then disconnects, with a phone or an iPad app,
then often you either have to shut the Pico W down completely for a few
seconds until the app realizes the Pico W is gone, or you have to
turn Bluetooth off on the Phone or iPad a while, before reconnection
is possible.

Sending the Active Sensing and the MIDI Clock message from the Pico W
to a remote device over Bluetooth is disabled. Either my code is not
always sending these messages correctly, there is some problem with
my ring buffer library corrupting the buffers, or the apps
I am using get overloaded with data, but it is too easy to get stuck
notes when these messages are enabled. Also, Active Sensing is sort
of a waste for USB and Bluetooth as there are other disconnection
detection mechanisms available. It would make more sense to support
active sensing locally over USB to help the USB keyboard detect
loss of Bluetooth connection. See the file `btstack_config.h` to
try enabling them if you need them.

Sometimes, with a USB MIDI keyboard playing a synthesizer app via
Bluetooth, if you play fast and use the pitch bend wheel and mod
wheel, notes will get stuck on in the synthesizer app. I am not
sure why.

On an iPad, using GarageBand Advanced Settings to connect to Bluetooth
will show an active Bluetooth connection but notes played on the
keyboard won't sound. However, if you start some other app such
as MIDI Wrench or AudioKit Synth One, play a note or two, exit
those apps and restart GarageBand, then notes play correctly in
GarageBand.

Some more detailed testing results are in this document.

# Hardware

![](ble-midi2usbhost.jpg)

The photo shows a Pico W with a microUSB to USB A adapter connected to an Arturia Keylab Essential 88 USB MIDI keyboard and wired to a USB C breakout board
connected to a 5000mAh phone charger battery. The keyboard is bus powered, so
the system draws enough power to prevent the phone charger battery from shutting
down automatically. An iPad running AudioKit Synth One is connected to the Pico W
via BLE-MIDI.

## Bill of Materials
I do not get money from hardware vendors. The links to hardware vendors are for illustration
and are not endorsements.

1. A Pico W board see the [Raspberry Pi website](https://www.raspberrypi.com/documentation/microcontrollers/raspberry-pi-pico.html) for more info
2. A USB C breakout board (e.g. [Adafruit 4090](https://www.adafruit.com/product/4090))
3. A microUSB OTG to USA A adapter (e.g. [Adafruit 1099](https://www.adafruit.com/product/1099))
4. Wire and solder
5. 5V USB C power supply (e.g., a battery or AC cell phone charger) and USB C charging cable
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
The following instructions assume that your Pico SDK code is stored in
`${PICO_SDK_PATH}` and that your project source tree will be in `${PICO_PROJ}/ble-midi2usbhost`. As of this writing, you will need to install a forked version
of the `tinyusb` library (See below).
## Getting the project source code
```
cd ${PICO_PROJ}
git clone https://github.com/rppicomidi/ble-midi2usbhost.git
cd ble-midi2usbhost
git submodule update --recursive --init
mkdir build
```
## Installing a tinyusb library that supports USB MIDI Host
The Pico SDK uses the main repository for `tinyusb` as a git submodule. Until the USB Host driver for MIDI is
incorporated in the main repository for `tinyusb`, you will need to use the latest development version in pull
request 1627 forked version. To make matters worse, this pull request is getting old and it is rapidly
diverging from the mainline of tinyusb, so I have been updating this pull request in my local workspace. I
have pushed my changes up to my own fork of `tinyusb` that I keep in a branch called `pr-midihost`. Sorry. I know. Yuk.

1. If you have not already done so, follow the instructions for installing the Raspberry Pi Pico SDK in Chapter 2 of the
[Getting started with Raspberry Pi Pico](https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf)
document. In particular, make sure `PICO_SDK_PATH` is set to the directory where you installed the pico-sdk.
2. Set the working directory to the tinyusb library
```
cd ${PICO_SDK_PATH}/lib/tinyusb
```
3. Create a git remote to point to my fork of tinyusb and get my updated branch of Pull Request 1627, which is called pr-midihost (short for pull-request-midihost)
```
git remote add rppicomidi https://github.com/rppicomidi/tinyusb.git
git fetch rppicomidi
```
4. Checkout the appropriate branch of my forked code
```
git checkout -b pr-midihost rppicomidi/pr-midihost
```
5. In case you ever need to get back to the official tinyusb code for some other
project, just check out the master branch and pull the latest code. Don't do this
for this this project.
```
git checkout master
git pull
```
## Build the Project
Make sure you have defined the following environment variables before either of these steps
```
export PICO_SDK_PATH=[insert the path to your Pico SDK here]
export PICO_BOARD=pico_w
```

To build from the command line, type these commands
```
cd ${PICO_PROJ}/ble-midi2usbhost/build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
```
This builds the debug version. If you want the release version, omit `-DCMAKE_BUILD_TYPE=Debug`. I have done all testing using the debug version.

The build generates the file `${PICO_PROJ}/ble-midi2usbhost/build/ble-midi2usbhost.uf2`, which you may then load to the Pico W board by mounting
the Pico W as a USB Mass Storage device and dragging the file to it, or you
may use the `picoprobe` to flash code.

## Using VisualStudio Code and a picoprobe
This is the workflow I normally use. This project already contains a .vscode directory that should work fine for you.
1. Start visual studio code in a way that imports the environment variables. From
the command line, after you set up the environment, type
```
code
```
2. Click `File->Open Folder...` and then select the folder where you installed
this project.
3. Choose the tool chain and build as described in Chapter 7 of the
[Getting started with Raspberry Pi Pico](https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf) document. Then load the code by running it as
described in the same chapter.

# About the Pairing model
This project as written uses "Just Works" pairing with bonding for maximum
compatibility. If you want to add more authentication features, I put the hooks
in the SM callback handler, but you will need to add more hardware for a display
(`IO_CAPABILITY_DISPLAY_ONLY`), a display and yes/no buttons (`IO_CAPABILITY_DISPLAY_YES_NO`),
or a keyboard (`IO_CAPABILITY_KEYBOARD_ONLY`). If you want to add
`IO_CAPABILITY_KEYBOARD_ONLY` I/O capability without dedicating a keyboard to the
device, you could add USB HID keyboard support to the USB Host configuration
and modify the software to handle HID reports. You would plug a USB HID keyboard
to the USB Host port of the ble-midi2usbhost hardware during pairing. Then
you would unplug the HID keyboard and plug in your MIDI device during
normal operation.

# Testing 23-Jul-2023
The ble-midi2usbhost code will cause the Pico W Bluetooth to advertise itself
as BLE-MIDI2USBH with a BLE-MIDI service UUID. It uses just works pairing. For
my testing, I used a Yamaha RefaceCS keyboard connected to the USB Host
port. I used software on the BLE-MIDI client to connect to the BLE-MIDI server
on the Pico W. Clients I used were MacOS (Monterey on a 2014 Mac Mini),
iPadOS 16.5.1(c) (on 7th generation iPad), and the WIDI BUD plugged into
a Windows 10 PC running Cubase 10.

## Pairing and re-connecting
No operating system I tried seemed to support pairing without use of
some sort of app. I found initial pairing works fine, but reconnection
is not reliable and usually requires some combination of power cycling
the ble-midi2usbhost hardware, restarting the app on the client device,
rebooting the client device, or some combination of these.

## Testing Keyboard->USB->BLE-MIDI->Client Syntheiszer
I then tried to play music on a synthesizer program that is running
on the BLE-MIDI client by pressing keys on the RefaceCS keyboard. Results varied.

### MacOS (Monterey on a 2014 Mac Mini)
I used the Audio MIDI Setup app to discover and connect the Pico W. I used GarageBand piano as the synthesizer. It worked as expected.

### iPadOS 16.5.1(c) (on 7th generation iPad)
Using the Bluetooth MIDI option in the Advanced settings of GarageBand could connect to the Pico W but music would not play. However, if I used the MIDI Wrench app to connect to the Pico W and then started the GarageBand App, it works, but not reliably. If you play fast, or use
the pitch bender, notes get stuck and things get unreliable.

AudioKit Synth One seemed to work fine most of the time but occasionally I was
getting stuck notes.

### WIDI BUD (on Windows 10 PC)
I plugged in the WIDI BUD. It discovered and connected to the Pico W. I started
Cubase and set up a project to trigger the HALion module from the WIDI BUD. It
worked well.

## Testing Client MIDI playback->BLE-MIDI->Keyboard's sound generator
### iPadOS 16.5.1(c) (on 7th generation iPad)
The keyboards on GarageBand and AudioKit Synth One do not seem to transmit MIDI.
However, the MIDI Wrench App keyboard seems to work correctly.

### TouchDAW 2.3.1 (on Android 13 Phone)
The keyboard app in TouchDAW 2.3.1 seems to work correctly as long as the connection
is good. However, I found myself doing a lot of rebooting hardware if connection is
lost. It is a good idea to exit the app using the Shutdown setting and then close
the app using the correct Android gesture. It is also a good idea to use the keep
device awake setting.
