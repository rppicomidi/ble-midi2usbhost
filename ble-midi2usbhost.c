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

#include <inttypes.h>
#include <stdio.h>
#include "midi_service_stream_handler.h"
#include "btstack.h"
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "pico/btstack_cyw43.h"
#include "ble-midi2usbhost.h"
#define _TUSB_HID_H_ // prevent tinyusb HID namespace conflicts with btstack
#include "bsp/board.h"
#include "tusb.h"
#include "class/midi/midi_host.h"
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
    0x0E, BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME, 'B', 'L', 'E', '-', 'M', 'I', 'D', 'I', '2', 'U', 'S', 'B', 'H',
};
const uint8_t scan_resp_data_len = sizeof(scan_resp_data);

static hci_con_handle_t con_handle = HCI_CON_HANDLE_INVALID;

static uint8_t midi_dev_addr = 0;

static void poll_usb_rx(bool connected)
{
    // device must be attached and have at least one endpoint ready to receive a message
    if (!connected || tuh_midih_get_num_rx_cables(midi_dev_addr) < 1)
    {
        return;
    }
    tuh_midi_read_poll(midi_dev_addr);
}

static void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    UNUSED(size);
    UNUSED(channel);
    bd_addr_t local_addr;
    uint8_t event_type;
    bd_addr_t addr;
    bd_addr_type_t addr_type;
    uint8_t status;
    switch(packet_type) {
        case HCI_EVENT_PACKET:
            event_type = hci_event_packet_get_type(packet);
            switch(event_type){
                case BTSTACK_EVENT_STATE:
                    if (btstack_event_state_get_state(packet) != HCI_STATE_WORKING) {
                        return;
                    }
                    gap_local_bd_addr(local_addr);
                    printf("BTstack up and running on %s.\n", bd_addr_to_str(local_addr));

                    // setup advertisements
                    uint16_t adv_int_min = 800;
                    uint16_t adv_int_max = 800;
                    uint8_t adv_type = 0;
                    bd_addr_t null_addr;
                    memset(null_addr, 0, 6);
                    gap_advertisements_set_params(adv_int_min, adv_int_max, adv_type, 0, null_addr, 0x07, 0x00);
                    assert(adv_data_len <= 31); // ble limitation
                    gap_advertisements_set_data(adv_data_len, (uint8_t*) adv_data);
                    assert(scan_resp_data_len <= 31); // ble limitation
                    gap_scan_response_set_data(scan_resp_data_len, (uint8_t*) scan_resp_data);
                    gap_advertisements_enable(1);

                    break;
                case HCI_EVENT_DISCONNECTION_COMPLETE:
                    printf("ble-midi2usbhost: HCI_EVENT_DISCONNECTION_COMPLETE event\r\n");
                    break;
                case HCI_EVENT_GATTSERVICE_META:
                    switch(hci_event_gattservice_meta_get_subevent_code(packet)) {
                        case GATTSERVICE_SUBEVENT_SPP_SERVICE_CONNECTED:
                            con_handle = gattservice_subevent_spp_service_connected_get_con_handle(packet);
                            break;
                        case GATTSERVICE_SUBEVENT_SPP_SERVICE_DISCONNECTED:
                            printf("ble-midi2usbhost: GATTSERVICE_SUBEVENT_SPP_SERVICE_DISCONNECTED event\r\n");
                            con_handle = HCI_CON_HANDLE_INVALID;
                            break;
                        default:
                            break;
                    }
                    break;
                case SM_EVENT_JUST_WORKS_REQUEST:
                    printf("ble-midi2usbhost: Just Works requested\n");
                    sm_just_works_confirm(sm_event_just_works_request_get_handle(packet));
                    break;
                case SM_EVENT_NUMERIC_COMPARISON_REQUEST:
                    printf("ble-midi2usbhost: Confirming numeric comparison: %"PRIu32"\n", sm_event_numeric_comparison_request_get_passkey(packet));
                    sm_numeric_comparison_confirm(sm_event_passkey_display_number_get_handle(packet));
                    break;
                case SM_EVENT_PASSKEY_DISPLAY_NUMBER:
                    printf("ble-midi2usbhost: Display Passkey: %"PRIu32"\n", sm_event_passkey_display_number_get_passkey(packet));
                    break;
                case SM_EVENT_IDENTITY_CREATED:
                    sm_event_identity_created_get_identity_address(packet, addr);
                    printf("ble-midi2usbhost: Identity created: type %u address %s\n", sm_event_identity_created_get_identity_addr_type(packet), bd_addr_to_str(addr));
                    break;
                case SM_EVENT_IDENTITY_RESOLVING_SUCCEEDED:
                    sm_event_identity_resolving_succeeded_get_identity_address(packet, addr);
                    printf("ble-midi2usbhost: Identity resolved: type %u address %s\n", sm_event_identity_resolving_succeeded_get_identity_addr_type(packet), bd_addr_to_str(addr));
                    break;
                case SM_EVENT_IDENTITY_RESOLVING_FAILED:
                    sm_event_identity_created_get_address(packet, addr);
                    printf("ble-midi2usbhost: Identity resolving failed\n");
                    break;
                case SM_EVENT_PAIRING_STARTED:
                    printf("Pairing started\n");
                    break;
                case SM_EVENT_PAIRING_COMPLETE:
                    switch (sm_event_pairing_complete_get_status(packet)){
                        case ERROR_CODE_SUCCESS:
                            printf("ble-midi2usbhost: Pairing complete, success\n");
                            break;
                        case ERROR_CODE_CONNECTION_TIMEOUT:
                            printf("ble-midi2usbhost: Pairing failed, timeout\n");
                            break;
                        case ERROR_CODE_REMOTE_USER_TERMINATED_CONNECTION:
                            printf("ble-midi2usbhost: Pairing failed, disconnected\n");
                            break;
                        case ERROR_CODE_AUTHENTICATION_FAILURE:
                            printf("ble-midi2usbhost: Pairing failed, authentication failure with reason = %u\n", sm_event_pairing_complete_get_reason(packet));
                            break;
                        default:
                            break;
                    }
                    break;
                case SM_EVENT_REENCRYPTION_STARTED:
                    sm_event_reencryption_complete_get_address(packet, addr);
                    printf("ble-midi2usbhost: Bonding information exists for addr type %u, identity addr %s -> re-encryption started\n",
                        sm_event_reencryption_started_get_addr_type(packet), bd_addr_to_str(addr));
                    break;
                case SM_EVENT_REENCRYPTION_COMPLETE:
                    switch (sm_event_reencryption_complete_get_status(packet)){
                        case ERROR_CODE_SUCCESS:
                            printf("ble-midi2usbhost: Re-encryption complete, success\n");
                            break;
                        case ERROR_CODE_CONNECTION_TIMEOUT:
                            printf("ble-midi2usbhost: Re-encryption failed, timeout\n");
                            break;
                        case ERROR_CODE_REMOTE_USER_TERMINATED_CONNECTION:
                            printf("ble-midi2usbhost: Re-encryption failed, disconnected\n");
                            break;
                        case ERROR_CODE_PIN_OR_KEY_MISSING:
                            printf("ble-midi2usbhost: Re-encryption failed, bonding information missing\n\n");
                            printf("Assuming remote lost bonding information\n");
                            printf("Deleting local bonding information to allow for new pairing...\n");
                            sm_event_reencryption_complete_get_address(packet, addr);
                            addr_type = sm_event_reencryption_started_get_addr_type(packet);
                            gap_delete_bonding(addr_type, addr);
                            break;
                        default:
                            break;
                    }
                    break;
                case GATT_EVENT_QUERY_COMPLETE:
                    status = gatt_event_query_complete_get_att_status(packet);
                    switch (status){
                        case ATT_ERROR_INSUFFICIENT_ENCRYPTION:
                            printf("ble-midi2usbhost: GATT Query failed, Insufficient Encryption\n");
                            break;
                        case ATT_ERROR_INSUFFICIENT_AUTHENTICATION:
                            printf("ble-midi2usbhost: GATT Query failed, Insufficient Authentication\n");
                            break;
                        case ATT_ERROR_BONDING_INFORMATION_MISSING:
                            printf("ble-midi2usbhost: GATT Query failed, Bonding Information Missing\n");
                            break;
                        case ATT_ERROR_SUCCESS:
                            printf("ble-midi2usbhost: GATT Query successful\n");
                            break;
                        default:
                            printf("ble-midi2usbhost: GATT Query failed, status 0x%02x\n", gatt_event_query_complete_get_att_status(packet));
                            break;
                    }
                    break;
                default:
                    break;
            } // event_type
            break;
        default:
            break;
    } // HCI_PACKET
}

static btstack_packet_callback_registration_t sm_event_callback_registration;
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
    l2cap_init();

    sm_init();

    att_server_init(profile_data, NULL, NULL);
    // just works, legacy pairing, with bonding
    sm_set_io_capabilities(IO_CAPABILITY_NO_INPUT_NO_OUTPUT);
    sm_set_authentication_requirements(SM_AUTHREQ_SECURE_CONNECTION | SM_AUTHREQ_BONDING);
    // register for SM events
    sm_event_callback_registration.callback = &packet_handler;
    sm_add_event_handler(&sm_event_callback_registration);
    midi_service_stream_init(packet_handler);

    // turn on bluetooth
    hci_power_control(HCI_POWER_ON);
    for(;;) {
        tuh_task();
        bool usb_connected = midi_dev_addr != 0 && tuh_midi_configured(midi_dev_addr);
        // poll the BLE-MIDI service and send MIDI data to USB
        if (con_handle != HCI_CON_HANDLE_INVALID && usb_connected && tuh_midih_get_num_tx_cables(midi_dev_addr) >= 1) {
            uint16_t timestamp;
            uint8_t mes[3];
            uint8_t nread = midi_service_stream_read(con_handle, sizeof(mes), mes, &timestamp);
            if (nread != 0) {
                // Ignore timestamps for now. Handling timestamps has a few issues:
                // 1. Some applications (e.g., TouchDAW 2.3.1 for Android or Midi Wrench on an iPad)
                //    always send timestamp value of 0.
                // 2. Synchronizing the timestamps to the system clock has issues if there are
                //    lost or out of order packets.
                uint32_t nwritten = tuh_midi_stream_write(midi_dev_addr, 0, mes, nread);
                if (nwritten != nread) {
                    TU_LOG1("Warning: Dropped %lu bytes receiving from UART MIDI In\r\n", nread - nwritten);
                }
            if (usb_connected)
                tuh_midi_stream_flush(midi_dev_addr);
            }
        }
        poll_usb_rx(usb_connected);
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
                    if (con_handle == HCI_CON_HANDLE_INVALID) {
                        TU_LOG1("ble-midi2usbhost: No BLE-MIDI connection: Dropped %lu bytes sending to BLE-MIDI\r\n", bytes_read - npushed);
                        return;
                    }
                    uint8_t npushed = midi_service_stream_write(con_handle, bytes_read, buffer);
                    if (npushed != bytes_read) {
                        TU_LOG1("ble-midi2usbhost: Warning: Dropped %lu bytes sending to BLE-MIDI\r\n", bytes_read - npushed);
                    }
                }
            }
        }
    }
}

void tuh_midi_tx_cb(uint8_t dev_addr)
{
    (void)dev_addr;
}
