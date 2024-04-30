#include <hidapi/hidapi_libusb.h>
#include <libusb-1.0/libusb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define VENDOR_ID 0x046D
#define PRODUCT_ID 0xC246
#define MOUSE_INTERFACE 1

/* ++ stolen from libratbag ^_^ */
#define HID_MAX_REPORT_SIZE 4096
struct logitech_g300_F0_report {
  uint8_t id;
  uint8_t unknown1 : 1;
  uint8_t resolution : 3;
  uint8_t profile : 4;
  uint8_t unknown2;
  uint8_t unknown3;
} __attribute__((packed));
/* -- from libratbg */

int main(void) {
  libusb_context *ctx;
  libusb_device_handle *devh;
  libusb_device *dev;
  hid_device *hdh;
  int stat;
  unsigned char color;

  stat = libusb_init(&ctx);
  if (stat < 0) {
    fprintf(stderr, "failed to init libusb | error: %s\n",
            libusb_error_name(stat));
    return -1;
  }

  stat = hid_init();
  if (stat < 0) {
    fprintf(stderr, "failed to init hidapi| error: %ls\n", hid_error(NULL));
    return -1;
  }

  devh = libusb_open_device_with_vid_pid(ctx, VENDOR_ID, PRODUCT_ID);
  if (devh == NULL) {
    fprintf(stderr, "failed to get device handle\n");
    return -1;
  }

  hdh = hid_open(VENDOR_ID, PRODUCT_ID, NULL);
  if (!hdh) {
    fprintf(stderr, "failed to open device using hidapi | error: %ls\n",
            hid_error(NULL));
    return -1;
  }

  dev = libusb_get_device(devh);
  if (dev == NULL) {
    fprintf(stderr, "failed to get device\n");
    libusb_close(devh);
    libusb_exit(ctx);
    return -1;
  }

  if (libusb_kernel_driver_active(devh, MOUSE_INTERFACE)) {
    stat = libusb_detach_kernel_driver(devh, MOUSE_INTERFACE);
    if (stat < 0) {
      fprintf(stderr, "failed to detach kernel driver | error: %s\n",
              libusb_error_name(stat));
      return -1;
    }
  }

  stat = libusb_claim_interface(devh, MOUSE_INTERFACE);
  if (stat < 0) {
    fprintf(stderr, "failed to claim interface");

    if (libusb_kernel_driver_active(devh, MOUSE_INTERFACE)) {
      stat = libusb_detach_kernel_driver(devh, MOUSE_INTERFACE);
      if (stat < 0) {
        fprintf(stderr, "failed to detach kernel driver | error: %s\n",
                libusb_error_name(stat));
        return -1;
      }
    }

    return -1;
  }

  unsigned char report[HID_MAX_REPORT_SIZE];
  unsigned char profile_id;

  memset(report, 0, sizeof(report));

  report[0] = 0xF0;

  stat = hid_get_feature_report(hdh, report, sizeof(report));
  if (stat < 0) {
    fprintf(stderr, "failed to get feature report | error: %ls\n",
            hid_error(hdh));
    return -1;
  }

  struct logitech_g300_F0_report *f0report =
      (struct logitech_g300_F0_report *)report;

  switch (f0report->profile) {
  case 0:
    profile_id = 0xF3;
    break;
  case 1:
    profile_id = 0xF4;
    break;
  case 2:
    profile_id = 0xF5;
    break;
  default:
    break;
  }

  memset(report, 0, sizeof(report));

  report[0] = profile_id;
  stat = hid_get_feature_report(hdh, report, sizeof(report));

  if (stat < 0) {
    fprintf(stderr, "failed to get active profile\n");
    return -1;
  }

  while(1) {

    report[1] = color;

    uint8_t bmRequestType = 0x21;
    uint8_t bRequest = 0x9;
    uint16_t wValue = 0x03F3;
    uint16_t wIndex = 0x1;

    int r = 0;

    if ((r = libusb_control_transfer(devh, bmRequestType, bRequest, wValue,
                                     wIndex, report, stat, 0)) < 0) {
      fprintf(stderr, "failed to set color | error: %s\n",
              libusb_error_name(r));
      stat = libusb_release_interface(devh, MOUSE_INTERFACE);
      if (stat < 0) {
        fprintf(stderr, "failed to release interface");
      }

      if (libusb_kernel_driver_active(devh, MOUSE_INTERFACE)) {
        stat = libusb_detach_kernel_driver(devh, MOUSE_INTERFACE);
        if (stat < 0) {
          fprintf(stderr, "failed to detach kernel driver | error: %s\n",
                  libusb_error_name(stat));
        }
      }

      libusb_close(devh);
      libusb_exit(ctx);
      return -1;
    }

    sleep(1);
    color = (color + 1) % 7;
  }
}
