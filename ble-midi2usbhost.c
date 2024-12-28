/**
 * MIT License
 *
 * Copyright (c) 2023 rppicomidi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

/**
 * To the extent this code is solely for use on the Rapsberry Pi Pico W or
 * Pico WH, the license file ${PICO_SDK_PATH}/src/rp2_common/pico_btstack/LICENSE.RP may
 * apply.
 * 
 */

/**
 * This file uses code from various BlueKitchen example files, which contain
 * the following copyright notice, included per the notice below.
 *
 * Copyright (C) 2018 BlueKitchen GmbH
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holders nor the names of
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 * 4. Any redistribution, use, or modification is done solely for
 *    personal benefit and not for any commercial purpose or for
 *    monetary gain.
 *
 * THIS SOFTWARE IS PROVIDED BY BLUEKITCHEN GMBH AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL BLUEKITCHEN
 * GMBH OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Please inquire about commercial licensing options at 
 * contact@bluekitchen-gmbh.com
 *
 */
#include <inttypes.h>
#include <stdio.h>
#include "ble_midi_server.h"
#include "btstack.h"
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "pico/btstack_cyw43.h"
#include "ble-midi2usbhost.h"
#define _TUSB_HID_H_ // prevent tinyusb HID namespace conflicts with btstack
#include "bsp/board_api.h"
#include "tusb.h"
#include "usb_midi_host.h"
// This is Bluetooth LE only
#define APP_AD_FLAGS 0x06
const uint8_t adv_data[] = {
    // Flags general discoverable
    0x02, BLUETOOTH_DATA_TYPE_FLAGS, APP_AD_FLAGS,
    // Service class list
    0x11, BLUETOOTH_DATA_TYPE_COMPLETE_LIST_OF_128_BIT_SERVICE_CLASS_UUIDS, 0x00, 0xc7, 0xc4, 0x4e, 0xe3, 0x6c, 0x51, 0xa7, 0x33, 0x4b, 0xe8, 0xed, 0x5a, 0x0e, 0xb8, 0x03,
};
const uint8_t adv_data_len = sizeof(adv_data);

const uint8_t scan_resp_data[] = {
    // Name
    0x11, BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME, 'B', 'L', 'E', '-', 'M', 'I', 'D', 'I', '2', 'U', 'S', 'B', 'H','U','B'
};
const uint8_t scan_resp_data_len = sizeof(scan_resp_data);

static uint8_t midi_dev_addr = 0;

int main()
{
    board_init();
    printf("Pico W BLE-MIDI to USB Host Adapter\r\n");
    tusb_init();
    // initialize CYW43 driver architecture (will enable BT if/because CYW43_ENABLE_BLUETOOTH == 1)
    if (cyw43_arch_init()) {
        printf("ble-midi2usbhost: failed to initialize cyw43_arch\n");
        return -1;
    }
    // No UI, just works pairing
    ble_midi_server_init(profile_data, scan_resp_data, scan_resp_data_len,
        IO_CAPABILITY_NO_INPUT_NO_OUTPUT,
        SM_AUTHREQ_SECURE_CONNECTION | SM_AUTHREQ_BONDING);

    for(;;) {
        tuh_task();
        bool usb_connected = midi_dev_addr != 0 && tuh_midi_configured(midi_dev_addr);
        // poll the BLE-MIDI service and send MIDI data to USB
        if (ble_midi_server_is_connected() && usb_connected && tuh_midih_get_num_tx_cables(midi_dev_addr) >= 1) {
            uint16_t timestamp;
            uint8_t mes[3];
            uint8_t nread = ble_midi_server_stream_read(sizeof(mes), mes, &timestamp);
            if (nread != 0) {
                // Ignore timestamps for now. Handling timestamps has a few issues:
                // 1. Some applications (e.g., TouchDAW 2.3.1 for Android or Midi Wrench on an iPad)
                //    always send timestamp value of 0.
                // 2. Synchronizing the timestamps to the system clock has issues if there are
                //    lost or out of order packets.
                uint32_t nwritten = tuh_midi_stream_write(midi_dev_addr, 0, mes, nread);
                if (nwritten != nread) {
                    TU_LOG1("Warning: Dropped %lu bytes receiving from Bluetooth MIDI In\r\n", nread - nwritten);
                }
            if (usb_connected)
                tuh_midi_stream_flush(midi_dev_addr);
            }
        }
    }
    return 0; // never gets here
}

//--------------------------------------------------------------------+
// TinyUSB Callbacks
//--------------------------------------------------------------------+

// Invoked when device with USB MIDI interface is mounted
void tuh_midi_mount_cb(uint8_t dev_addr, uint8_t in_ep, uint8_t out_ep, uint8_t num_cables_rx, uint16_t num_cables_tx)
{
  printf("ble-midi2usbhost: MIDI device address = %u, IN endpoint %u has %u cables, OUT endpoint %u has %u cables\r\n",
      dev_addr, in_ep & 0xf, num_cables_rx, out_ep & 0xf, num_cables_tx);

  if (midi_dev_addr == 0) {
    // then no MIDI device is currently connected
    midi_dev_addr = dev_addr;
  }
  else {
    printf("ble-midi2usbhost: A different USB MIDI Device is already connected.\r\nOnly one device at a time is supported in this program\r\nDevice is disabled\r\n");
  }
}

// Invoked when device with hid interface is un-mounted
void tuh_midi_umount_cb(uint8_t dev_addr, uint8_t instance)
{
  if (dev_addr == midi_dev_addr) {
    midi_dev_addr = 0;
    printf("ble-midi2usbhost: MIDI device address = %d, instance = %d is unmounted\r\n", dev_addr, instance);
  }
  else {
    printf("ble-midi2usbhost: Unused MIDI device address = %d, instance = %d is unmounted\r\n", dev_addr, instance);
  }
}

void tuh_midi_rx_cb(uint8_t dev_addr, uint32_t num_packets)
{
    if (midi_dev_addr == dev_addr)
    {
        if (num_packets != 0)
        {
            uint8_t cable_num;
            uint8_t buffer[48];
            while (1) {
                uint32_t bytes_read = tuh_midi_stream_read(dev_addr, &cable_num, buffer, sizeof(buffer));
                if (bytes_read == 0)
                    return; // done
                if (cable_num == 0) {
                    if (!ble_midi_server_is_connected()) {
                        TU_LOG1("ble-midi2usbhost: No BLE-MIDI connection: Dropped %lu bytes sending to BLE-MIDI\r\n", bytes_read);
                        return;
                    }
                    async_context_acquire_lock_blocking(cyw43_arch_async_context());
                    uint8_t npushed = ble_midi_server_stream_write(bytes_read, buffer);
                    if (npushed != bytes_read) {
                        TU_LOG1("ble-midi2usbhost: Warning: Dropped %lu bytes sending to BLE-MIDI\r\n", bytes_read - npushed);
                    }
                    async_context_release_lock(cyw43_arch_async_context());
                }
            }
        }
    }
}

void tuh_midi_tx_cb(uint8_t dev_addr)
{
    (void)dev_addr;
}
