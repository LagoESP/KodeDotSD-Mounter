#include "Storage.h"

// ---- Your SD pin map (1-bit) ----
static constexpr int PIN_SD_CLK = 6;  // CLK
static constexpr int PIN_SD_CMD = 5;  // CMD
static constexpr int PIN_SD_D0  = 7;  // D0

static USBMSC s_msc;
static bool   s_mounted   = false;   // our own truth for “presented as drive”
static bool   s_usbOnline = false;   // driven by USB.onEvent()

// ---------------- MSC callbacks ----------------
static int32_t onRead(uint32_t lba, uint32_t offset, void *buffer, uint32_t bufsize) {
  const uint32_t sec = SD_MMC.sectorSize();
  if (!sec || !bufsize) return -1;

  uint8_t* dst   = static_cast<uint8_t*>(buffer);
  uint32_t remain = bufsize;
  uint32_t blk    = lba;
  uint32_t off    = offset;

  while (remain) {
    uint8_t tmp[512];
    if (!SD_MMC.readRAW(tmp, blk)) return -1;

    uint32_t chunk = min(remain, sec - off);
    memcpy(dst, tmp + off, chunk);

    dst    += chunk;
    remain -= chunk;
    off     = 0;
    blk    += 1;
  }
  return (int32_t)bufsize;
}

static int32_t onWrite(uint32_t lba, uint32_t offset, uint8_t *buffer, uint32_t bufsize) {
  const uint32_t sec = SD_MMC.sectorSize();
  if (!sec || !bufsize) return -1;

  uint8_t* src   = buffer;
  uint32_t remain = bufsize;
  uint32_t blk    = lba;
  uint32_t off    = offset;

  while (remain) {
    uint8_t tmp[512];
    if (off != 0 || remain < sec) {
      if (!SD_MMC.readRAW(tmp, blk)) return -1;  // read-modify-write for partials
    }
    uint32_t chunk = min(remain, sec - off);
    memcpy(tmp + off, src, chunk);
    if (!SD_MMC.writeRAW(tmp, blk)) return -1;

    src    += chunk;
    remain -= chunk;
    off     = 0;
    blk    += 1;
  }
  return (int32_t)bufsize;
}

static bool onStartStop(uint8_t /*power_condition*/, bool start, bool load_eject) {
  // Host may ask us to stop/eject; honor it by dropping media on next loop.
  if (!start && load_eject) {
    Storage::unmount();
  }
  return true;
}

// ---------------- USB event wiring ----------------
static void usbEventCallback(void*, esp_event_base_t base, int32_t id, void* event_data) {
  if (base != ARDUINO_USB_EVENTS) return;
  switch (id) {
    case ARDUINO_USB_STARTED_EVENT:
    case ARDUINO_USB_RESUME_EVENT:
      s_usbOnline = true;  break;
    case ARDUINO_USB_SUSPEND_EVENT:
    case ARDUINO_USB_STOPPED_EVENT:
      s_usbOnline = false; break;
    default: break;
  }
}

namespace Storage {

void attachUsbEvents() {
  // Use official Arduino-ESP32 USB events instead of usb_serial_jtag_is_connected().
  // This tracks the TinyUSB device state (MSC/CDC/HID) and is robust across unmounts.  // docs show event list
  USB.onEvent(usbEventCallback);  // ARDUINO_USB_* events. 
  // (You still call USB.begin() when you actually want to enumerate your device.)
}

bool mount() {
  if (s_mounted) return true;

  // 1) Mount SD in 1-bit mode with your custom pins
  SD_MMC.setPins(PIN_SD_CLK, PIN_SD_CMD, PIN_SD_D0);
  if (!SD_MMC.begin("/sdcard", /*1-bit*/ true)) {
    return false;
  }
  if (SD_MMC.cardType() == CARD_NONE) {
    SD_MMC.end();
    return false;
  }

  // 2) Configure MSC and present media
  s_msc.vendorID("ESP32");
  s_msc.productID("SD-USB");
  s_msc.productRevision("1.0");
  s_msc.onRead(onRead);
  s_msc.onWrite(onWrite);
  s_msc.onStartStop(onStartStop);

  const uint32_t secSize  = SD_MMC.sectorSize();
  const uint32_t secCount = SD_MMC.numSectors();
  if (!secSize || !secCount) {
    SD_MMC.end();
    return false;
  }

  s_msc.mediaPresent(true);
  if (!s_msc.begin(secCount, secSize)) {   // start MSC class
    SD_MMC.end();
    return false;
  }

  // 3) Start the USB device stack (enumeration); safe to call more than once
  USB.begin();   // API docs: common USB begin/start; events fire via onEvent
  s_mounted = true;
  return true;
}

void unmount() {
  if (!s_mounted) return;

  // Tell the host the drive is going away, then stop MSC and free the SD bus.
  // These APIs are the officially documented way to remove MSC. 
  s_msc.mediaPresent(false);     // signal “no media” to host first
  delay(50);                     // short debounce window for host to close handles
  s_msc.end();                   // release MSC class resources
  delay(10);
  SD_MMC.end();                  // release SDMMC bus/pins

  s_mounted = false;

  // NOTE: We intentionally DO NOT call any “USB end” here.
  // Arduino-ESP32 documents MSC::end() but USB has only begin()+events;
  // keeping USB active preserves event notifications for future mounts.
}

bool isMounted()   { return s_mounted; }
bool isUsbOnline() { return s_usbOnline; }

} // namespace Storage
