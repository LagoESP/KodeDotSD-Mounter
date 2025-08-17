#pragma once
#include <Arduino.h>
#include "USB.h"
#include "USBMSC.h"
#include "SD_MMC.h"

// Simple facade to expose SD as USB Mass Storage (MSC)
namespace Storage {

  // Call once to wire USB events (optional, but recommended if you auto-mount)
  void attachUsbEvents();

  // Mounts SD (1-bit on GPIO6/5/7) and presents it as a USB drive.
  // Returns true on success.
  bool mount();

  // Gracefully removes the USB drive from the host and releases the SD bus.
  // Safe to call if not mounted.
  void unmount();

  // Lightweight getters for your UI:
  bool isMounted();            // true after successful mount() and before unmount()
  bool isUsbOnline();          // reflects USB STARTED/RESUME vs STOPPED/SUSPEND (from events)

} // namespace Storage
