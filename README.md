# ble-midi2usbhost
A Pico W/Pico2 W Bluetooth LE MIDI to USB Host adapter for any class compliant USB MIDI device

This project uses the native Raspberry Pi Pico W or Pico2 W USB port as a USB MIDI host port
and it uses the Bluetooth LE capability of the board's CYW43 WiFi/Bluetooth module
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

If you have problems or need help, please file issues in the Issues tab on the project
[home page](https://github.com/rppicomidi/ble-midi2usbhost). If you have specific
Bluetooth issues and can attach an air trace, that can help a lot. If
you find a bug and have a fix for it, pull requests are welcome.

This project has been tested with Pico SDK 2.2.0 and TinyUSB 0.18.0. It has been
tested with a Pico W board and a Pico2 W board.

# Known Issues
## 26-Jul-2023:
### iPad GarageBand
The iPad GarageBand app Advanced Settings shows the ble-midi2usbhost Pico W Bluetooth successfully connected, but MIDI data does not play GarageBand virtual instruments.

- Workaround: Use a different iPad BLE-MIDI app like MIDI Wrench or
Synth One to connect to the ble-midi2usbhost Pico W Bluetooth first.
After that, GarageBand virtual instruments will recognize BLE-MIDI messages.

In my mind, this is an issue with the GarageBand App. GarageBand on my
Mac Mini running Monterey works fine. So too do all other BLE-MIDI apps I
have tested on my iPad. If this is wrong, please file an issue in this
project.

### iPad Bluetooth Disconnect Does Not Fully Disconnect
If the Pico W pairs, then disconnects, with an iPad app, you have
to fully exit the app and wait a few seconds before you can connect
again, or you have to shut off iPad Bluetooth for about a second,
then turn it on. It seems the iPad only disconnects from the GATT
layer but does not fully disconnect from the Pico W Bluetooth.

By contrast, on my Mac Mini running Monterey, disconnect fully
disconnects, so reconnecting is reliable.

### Android Bluetooth Reconnection unreliable
If the Pico W Bluetooth pairs, then disconnects, from the Android App
TouchDAW, reconnection does not work correctly. Sometimes you have
to reboot the phone to get it to connect again. I am not sure if the
issue is with ble-midi2usbhost, my cheap phone, TouchDAW, or some
combination of these.

### Active Sensing is Disabled
If the MIDI keyboard or other device connected via the ble-midi2usbhost
USB Host port sends the Active Sensing message, ble-midi2usbhost
filters it out. This is done on purpose. In the future, Active Sensing
will be supported locally. We don't need to waste Bluetooth bandwidth
on this message.

### Can't Pair with Linux
I have successfully used Linux Mint 22.1 and `bluetoothctl` to pair
my Pico2 W board with my computer. Your mileage may vary.

Most Linux distributions use the BlueZ Bluetooth stack. By default,
in 2023, BLE-MIDI is disabled. Enabling requires rebuilding the BlueZ stack
with the `--enable-midi` option. You can find old instructions for
doing this on the Internet. I have not tried this.

### Can't Pair with Windows
I believe BLE-MIDI is not supported in Windows unless the device has its own
device driver installed or you are using software like Cubase that supports
WinRT MIDI. I have not tried this.

# Hardware

![](ble-midi2usbhost.jpg)

The photo shows a Pico W with a microUSB to USB A adapter connected to an Arturia Keylab Essential 88 USB MIDI keyboard. The Pico W is also wired to a USB C 
breakout board connected to a USB C to USB A cable that is
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
7. 2-pin 0.1" spacing square pin header for connecting the picoprobe UART pins (optional)
8. A Pico board board programmed as a picoprobe and populated with headers for wiring to the Pico W board (optional)
9. 5 Jumper wires for connecting the picoprobe to the Pico W (optional)
10. A microUSB cable to connect the picoprobe to your debug host computer (optional)

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
`${PICO_SDK_PATH}` and that your project source tree will be in `${PICO_PROJ}/ble-midi2usbhost`.
If you are using Pico-SDK before version 2.2.0, you will need to update your
version of the TinyUSB library that ships with the Pico SDK. See below.
## Getting the project source code
```
cd ${PICO_PROJ}
git clone https://github.com/rppicomidi/ble-midi2usbhost.git
cd ble-midi2usbhost
git submodule update --recursive --init
mkdir build
```
When you are done with these steps, your source tree should look like this
(library directories, but not files, are shown for brevity).
```
ble-midi2usbhost/
    |
    +--lib
    |   |
    |   +--pico-w-ble-midi-lib/
    |   |
    |   +--ring-buffer-lib/
    |   |
    |   +--usb_midi_host
    |
    +--ble-midi2usbhost.c
    |
    +--ble-midi2usbhost.gatt
    |
    +--ble-midi2usbhost.jpg
    |
    +--btstack_config.h
    |
    +--CMakeLists.txt
    |
    +--LICENSE
    |
    +--pico_sdk_import.cmake
    |
    +--README.md
    |
    +--tusb_config.h
```
## Installing a tinyusb library that supports the usb_midi_host application library
The Pico SDK uses the main repository for `tinyusb` as a git submodule. Pico 
SDK 1.5.1 configured to use version 0.15.0 of TinyUSB. That version does not
support application host drivers. You need a version of TinyUSB from 15-Aug-2023
or later. To make sure you have that, follow these instructions

1. If you have not already done so, follow the instructions for installing the Raspberry Pi Pico SDK in Chapter 2 of the
[Getting started with Raspberry Pi Pico](https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf)
document. In particular, make sure `PICO_SDK_PATH` is set to the directory where you installed the pico-sdk and you have installed all of the git submodules.
2. Set the working directory to the tinyusb library
```
cd ${PICO_SDK_PATH}/lib/tinyusb
```
3. Check out the master branch and make sure you are using version 0.18.0
```
git checkout master
git pull
git checkout 0.18.0
```
## Build the Project
Make sure you have defined the following environment variables before either of these steps
```
export PICO_SDK_PATH=[insert the path to your Pico SDK here]
export PICO_BOARD=pico_w
```
If you are using a Pico2 W board, replace `pico_w` with `pico2_w`.

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

## Using VisualStudio Code and a picoprobe (with a pre-version 2.0.0-type VS Code plugins)
This is the workflow I normally use. This project already contains a .vscode directory that should work fine for you.
I started developing using the [older version of Getting started with Raspberry Pi Pico](https://archive.org/details/getting_started_with_pico/page/1/mode/2up),
and I have not changed my system much. If you use Pico SDK version 2.0.0-type VS Code plugins, skip this section.
1. Set up the environment variables and an instance of openocd as described
in the [older version of Getting started with Raspberry Pi Pico](https://archive.org/details/getting_started_with_pico/page/1/mode/2up), document.
2. Start visual studio code in a way that imports the environment variables. From
the command line, after you set up the environment, type
```
code
```
3. Click `File->Open Folder...` and then select the folder where you installed
this project.
4. Choose the tool chain and build as described in Chapter 6 of the old version of
[Getting started with Raspberry Pi Pico](https://archive.org/details/getting_started_with_pico/page/1/mode/2up) document. Then load the code by running it as
described in the same chapter.

## Using VisualStudio Code and a debugprobe (with the official Pico SDK VS Code plugin)
1. Import the project
2. Choose the board type to match your Pico W or Pico2 W board
3. Build the project
4. Debug the project
See [Getting started with Raspberry Pi Pico-series](https://pip-assets.raspberrypi.com/categories/610-raspberry-pi-pico/documents/RP-008276-DS-1-getting-started-with-pico.pdf?disposition=inline)

# Using the software
If you have many active Bluetooth MIDI devices in your area, it may be helpful to know the Bluetooth address of you Pico W or Pico2 W board.
When the software starts up, the serial terminal connected to UART 0 (Pico W/Pico2 W board pins 1 and 2) will display the Bluetooth
address of your board. The serial port terminal will look something like this:
```
Pico W BLE-MIDI to USB Host Adapter
Version: 7.95.61 (abcd531 CY) CRC: 4528a809 Date: Wed 2023-01-11 10:29:38 PST Ucode Ver: 1043.2169 FWID 01-7afb0879
cyw43 loaded ok, mac [Your device's MAC address]
BT FW download, version = CYW4343A2_001.003.016.0031.0000_Generic_SDIO_37MHz_wlbga_BU_dl_signed
BTstack up and running on [Your device's Bluetooth address]
```

## Testing using an iPad
I have tested this using an iPad running the Midi Wrench app. To pair with the iPad, plug in the Pico W board to a power source.
The Pico W will advertise itself as a MIDI device and its Bluetooth address will show up during discovery.
Select it to pair. Once it is paired, it will show up on the device list as "BLE-MIDI2USBHUB". You can then use
MIDI Wrench as an agent to work with Garage Band and other programs. The AudioKit Synth One app Bluetooth button will also allow you to discover and pair your iPad with the Pico board.


## Testing using Linux
You can also from Linux use the `bluetoothctl` command to discover the device.
```
$ bluetoothctl
[bluetooth]# scan.uuids 03B80E5A-EDE8-4B33-A751-6CE34EC4C700
[bluetooth]# scan le
```
The long number is the Bluetooth LE MIDI serive UUID. The scan will display the
Bluetooth address of the MIDI device. If you omit that line, you will probably
get a flood of Bluetooth addresses that are hard to figure out. I do not
use `bluetoothctl` to connect to the device because I use it on my iPad, but
`bluetoothctl` has commands to pair and connect. Whether your Linux kernel supports
Bluetooth MIDI and makes it available is dependent on your Linux distribution.
```
[bluetooth]# scan stop
[bluetooth]# pair [your device's Bluetooth address]
[bluetooth]# connect [your device's Bluetooth address]
[bluetooth]# exit
```
You can list the available MIDI playback and record devices with
```
aplaymidi -l
arecordmidi -l
```
The `amidi -l` command will not show any devices.

## Other operating systems
If you use this project under Windows, Android, MacOS, or others, please submit
a pull request to tell users how to get going with this software.

# Troubleshooting
If your project works for some USB MIDI devices and not others, one
thing to check is the size of buffer to hold USB descriptors and other
data used for USB enumeration. Look in the file `tusb_config.h` for
```
#define CFG_TUH_ENUMERATION_BUFSIZE 512
```
Very complex MIDI devices or USB Audio+MIDI devices like DSP guitar pedals
or MIDI workstation keyboards may have large USB configuration descriptors.
This project assumes 512 bytes is enough, but it may not be for your device.

To check if the descriptor size is the issue, use your development computer to
dump the USB descriptor for your device and then add up the wTotalLength field
values for each configuration in the descriptor.


For Linux and MacOS Homebrew, the command is lsusb -d [vid]:[pid] -v
For Windows, it is simplest to install a program like
[Thesycon USB Descriptor Dumper](https://www.thesycon.de/eng/usb_descriptordumper.shtml).

For example, this is the important information from `lsusb -d 0944:0117 -v`
from a Korg nanoKONTROL2:
```
  bNumConfigurations      1
  Configuration Descriptor:
    bLength                 9
    bDescriptorType         2
    wTotalLength       0x0053
    bNumInterfaces          1
    bConfigurationValue     1
    iConfiguration          0 
    bmAttributes         0x80
      (Bus Powered)
    MaxPower              100mA
```
This is the important information from the Thesycon USB Descriptor Dumper for
a Valeton NUX MG-400
```
0x01	bNumConfigurations

Device Qualifier Descriptor is not available. Error code: 0x0000001F


-------------------------
Configuration Descriptor:
-------------------------
0x09	bLength
0x02	bDescriptorType
0x0158	wTotalLength   (344 bytes)
0x04	bNumInterfaces
0x01	bConfigurationValue
0x00	iConfiguration
0xC0	bmAttributes   (Self-powered Device)
0x00	bMaxPower      (0 mA)
```
You can see that if `CFG_TUH_ENUMERATION_BUFSIZE` were 256 instead of 512,
the Korg nanoKONTROL2 would have no trouble enumerating but the Valeton
NUX MG-400 would fail because TinyUSB couldn't load the whole configuration
descriptor to memory.

# Tweaking the Pairing Model
This project as written uses "Just Works" pairing with bonding for maximum
ease of use. If you want to add more authentication features, I put the hooks
in the SM callback handler, but you will need to add more hardware for a display
(`IO_CAPABILITY_DISPLAY_ONLY`), a display and yes/no buttons (`IO_CAPABILITY_DISPLAY_YES_NO`),
or a keyboard (`IO_CAPABILITY_KEYBOARD_ONLY`). If you want to add
`IO_CAPABILITY_KEYBOARD_ONLY` I/O capability without dedicating a keyboard to the
device, you could add USB HID keyboard support to the USB Host configuration
and modify the software to handle HID reports. You would plug a USB HID keyboard
to the USB Host port of the ble-midi2usbhost hardware during pairing. Then
you would unplug the HID keyboard and plug in your MIDI device during
normal operation.

If you want to disable legacy pairing in the name of more security, add the line
```
sm_set_secure_connections_only_mode(true);
```
before any other `sm_` function calls.

If you want to use a packet sniffer to get an air trace of the BLE-MIDI stream,
it is simpler if you use legacy pairing. To do that, in `ble-midi2usbhost.c`,
change the line
```
sm_set_authentication_requirements(SM_AUTHREQ_SECURE_CONNECTION | SM_AUTHREQ_BONDING);
```
to
```
sm_set_authentication_requirements(SM_AUTHREQ_BONDING);
```
.
