/**
 * SD Card Information Display for Kode Dot
 * Simple display of SD card metrics with USB detection.
 */
/* ───────── KODE | docs.kode.diy ───────── */
#include <Arduino.h>
#include <kodedot/display_manager.h>
#include <kodedot/pin_config.h>
#include <lvgl.h>
#include <SD_MMC.h>
#include <FS.h>
#include <driver/usb_serial_jtag.h>
#include <Storage.h>
#include <Adafruit_NeoPixel.h>

// ───────── Visuals ─────────
#define COLOR_GREY_BTN   0x666666
#define COLOR_GREY_TEXT  0xAAAAAA
#define COLOR_ORANGE     0xFF7F1F
#define COLOR_GREEN      0x4CAF50
#define COLOR_WHITE      0xFFFFFF
#define COLOR_RED        0xFF6B6B
#define COLOR_PURE_GREEN 0x00FF00  // Pure green for NeoPixel

// Include Inter fonts and logo
extern const lv_font_t Inter_50;
extern const lv_font_t Inter_30;
extern const lv_font_t Inter_20;
extern const lv_image_dsc_t logotipo;

// Display manager
DisplayManager display;

// NeoPixel control
Adafruit_NeoPixel *pixels;

// ───────── SD card info struct ─────────
struct SDCardInfo {
    bool detected = false;
    uint64_t totalBytes = 0;
    uint64_t usedBytes = 0;
    int folderCount = 0;
    int rootFileCount = 0;
    int totalFileCount = 0;
};

// ───────── UI ─────────
lv_obj_t *logo_img;
lv_obj_t *status_label;
lv_obj_t *storage_label;
lv_obj_t *folders_label;
lv_obj_t *root_files_label;
lv_obj_t *total_files_label;

lv_obj_t *mount_btn;
lv_obj_t *mount_btn_label;

// ───────── USB Detection ─────────
bool usb_connected = false;
bool last_usb_state = false;
bool usb_was_connected_before_mount = false;  // Track USB state before mounting
bool usb_was_ever_mounted = false;

// ───────── Mount State ─────────
bool sd_card_mounted = false;

// ───────── Function declarations ─────────
void createSDCardScreen();
void refreshSDCardInfo();
SDCardInfo getSDCardInfo();
void countFilesAndFolders(const char* path, SDCardInfo& info, bool isRoot = false);
String formatBytes(uint64_t bytes);

void updateMountButtonState();
bool isUSBConnected();
void updateNeoPixel(uint32_t color);

// ───────── Timing ─────────
unsigned long lastRefreshTime = 0;

unsigned long lastUSBCheckTime = 0;
const unsigned long REFRESH_INTERVAL = 500;

const unsigned long USB_CHECK_INTERVAL = 100;  // Check USB every 100ms



// ───────── USB Detection Function ─────────
bool isUSBConnected() {
    // If USB was ever mounted, return true
    if (usb_was_ever_mounted) {
        return true;
    }
    // If SD card is mounted, return the preserved USB state
    if (sd_card_mounted) {
        return usb_was_connected_before_mount;
    }
    // Otherwise, check current USB connection
    return usb_serial_jtag_is_connected();
}


// ───────── Setup/loop ─────────
void setup() {
    Serial.begin(115200);
    Serial.println("SD Card Info Display with USB Detection starting...");
    
    // Initialize NeoPixel
    pixels = new Adafruit_NeoPixel(NEO_PIXEL_COUNT, NEO_PIXEL_PIN, NEO_GRB + NEO_KHZ800);
    pixels->begin();
    pixels->clear();
    pixels->show();
    
    if (!display.init()) {
        Serial.println("Error: Failed to initialize display");
        while (1) { delay(1000); }
    }
    
    createSDCardScreen();
    Serial.println("SD Card screen ready!");
}

void loop() {
    display.update();
    const unsigned long now = millis();
    
    // Check USB connection status
    if (now - lastUSBCheckTime >= USB_CHECK_INTERVAL) {
        bool current_usb_state = isUSBConnected();
        
        // Only update USB state if not mounted, or if we're detecting initial connection
        if (!sd_card_mounted) {
            usb_connected = current_usb_state;
            
            // Update button if USB state changed
            if (usb_connected != last_usb_state) {
                if (usb_connected) {
                    Serial.println("USB Connected!");
                } else {
                    Serial.println("USB Disconnected!");
                }
                updateMountButtonState();
                last_usb_state = usb_connected;
            }
        }
        lastUSBCheckTime = now;
    }
    
    if (now - lastRefreshTime >= REFRESH_INTERVAL) {
        refreshSDCardInfo();
        lastRefreshTime = now;
    }


    
    delay(5);
}

// ───────── UI ─────────
static void mount_btn_event_handler(lv_event_t * e) {
    if (usb_connected && !sd_card_mounted) {
        // USB is connected - Mount SD Card action
        Serial.println("Mount SD Card button pressed");
        
        // Hide storage information
        lv_obj_add_flag(storage_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(folders_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(root_files_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(total_files_label, LV_OBJ_FLAG_HIDDEN);
        
        // Change status to "SD Card in Mount Mode"
        lv_label_set_text(status_label, "SD Card in Mount Mode");
        lv_obj_set_style_text_color(status_label, lv_color_hex(COLOR_ORANGE), 0);
        
        // Change button to green with "Unmount SD Card" text
        lv_obj_set_style_bg_color(mount_btn, lv_color_hex(COLOR_GREEN), 0);
        lv_obj_set_style_text_color(mount_btn_label, lv_color_hex(COLOR_WHITE), 0);
        lv_label_set_text(mount_btn_label, "Unmount SD Card");
        
        // Set NeoPixel to pure green
        updateNeoPixel(COLOR_PURE_GREEN);
        
        // Set mount state
        sd_card_mounted = true;
        usb_was_connected_before_mount = usb_connected;  // Preserve USB state
        
        Storage::mount();

        usb_was_ever_mounted = true;
        
    } else if (usb_connected && sd_card_mounted) {
        // Already mounted - Unmount SD Card action
        Serial.println("Unmount SD Card button pressed");
        
        // Unmount the SD card
        Storage::unmount();
        
        // Reset mount state
        sd_card_mounted = false;
        usb_was_connected_before_mount = false;
        
        // Restore normal display
        refreshSDCardInfo();
        
        // Update button back to "Mount SD Card" (this will also update NeoPixel to orange)
        updateMountButtonState();
        
    } else {
        // USB not connected - show message or refresh
        Serial.println("USB not connected - cannot mount SD card");
        refreshSDCardInfo();
    }
}


void updateMountButtonState() {
    // First check if SD card is detected
    bool sd_card_detected = false;
    if (!sd_card_mounted) {
        // Only check SD card if not currently mounted
        // We'll get this info from refreshSDCardInfo which runs every 500ms
        SDCardInfo temp_info = getSDCardInfo();
        sd_card_detected = temp_info.detected;
    } else {
        sd_card_detected = true; // If mounted, we know it was detected
    }
    
    if (usb_connected && !sd_card_mounted && sd_card_detected) {
        // USB connected, SD card detected, not mounted: Orange button with "Mount SD Card"
        lv_obj_set_style_bg_color(mount_btn, lv_color_hex(COLOR_ORANGE), 0);
        lv_obj_set_style_text_color(mount_btn_label, lv_color_hex(COLOR_WHITE), 0);
        lv_label_set_text(mount_btn_label, "Mount SD Card");
        lv_obj_clear_state(mount_btn, LV_STATE_DISABLED);
        
        // Set NeoPixel to orange
        updateNeoPixel(COLOR_ORANGE);
        
    } else if (usb_connected && !sd_card_mounted && !sd_card_detected) {
        // USB connected, no SD card detected: Grey button with "No SD Card"
        lv_obj_set_style_bg_color(mount_btn, lv_color_hex(COLOR_GREY_BTN), 0);
        lv_obj_set_style_text_color(mount_btn_label, lv_color_hex(COLOR_GREY_TEXT), 0);
        lv_label_set_text(mount_btn_label, "No SD Card");
        lv_obj_add_state(mount_btn, LV_STATE_DISABLED);
        
        // Turn off NeoPixel
        updateNeoPixel(0x000000);
        
    } else if (usb_connected && sd_card_mounted) {
        // USB connected, already mounted: Green button with "Unmount SD Card"
        lv_obj_set_style_bg_color(mount_btn, lv_color_hex(COLOR_GREEN), 0);
        lv_obj_set_style_text_color(mount_btn_label, lv_color_hex(COLOR_WHITE), 0);
        lv_label_set_text(mount_btn_label, "Unmount SD Card");
        lv_obj_clear_state(mount_btn, LV_STATE_DISABLED);
        
        // Set NeoPixel to pure green
        updateNeoPixel(COLOR_PURE_GREEN);
        
    } else {
        // USB not connected: Grey button with "Connect USB C to PC"
        lv_obj_set_style_bg_color(mount_btn, lv_color_hex(COLOR_GREY_BTN), 0);
        lv_obj_set_style_text_color(mount_btn_label, lv_color_hex(COLOR_GREY_TEXT), 0);
        lv_label_set_text(mount_btn_label, "Connect USB C to PC");
        lv_obj_clear_state(mount_btn, LV_STATE_DISABLED);
        
        // Turn off NeoPixel
        updateNeoPixel(0x000000);
        
        // Reset mount state when USB disconnects
        if (sd_card_mounted) {
            sd_card_mounted = false;
            usb_was_connected_before_mount = false;  // Reset preserved state
            // Show storage info again when USB disconnects
            refreshSDCardInfo();
        }
    }
    

}


void createSDCardScreen() {
    lv_obj_t * scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);

    logo_img = lv_image_create(scr);
    lv_image_set_src(logo_img, &logotipo);
    lv_obj_align(logo_img, LV_ALIGN_TOP_MID, 0, 10);

    status_label = lv_label_create(scr);
    lv_obj_set_style_text_color(status_label, lv_color_hex(COLOR_WHITE), 0);
    lv_obj_set_style_text_font(status_label, &Inter_30, 0);
    lv_obj_align(status_label, LV_ALIGN_TOP_MID, 0, 95);

    storage_label = lv_label_create(scr);
    lv_obj_set_style_text_color(storage_label, lv_color_hex(0x999999), 0);
    lv_obj_set_style_text_font(storage_label, &Inter_20, 0);
    lv_obj_align(storage_label, LV_ALIGN_TOP_MID, 0, 140);

    folders_label = lv_label_create(scr);
    lv_obj_set_style_text_color(folders_label, lv_color_hex(0x999999), 0);
    lv_obj_set_style_text_font(folders_label, &Inter_20, 0);
    lv_obj_align(folders_label, LV_ALIGN_TOP_MID, 0, 170);

    root_files_label = lv_label_create(scr);
    lv_obj_set_style_text_color(root_files_label, lv_color_hex(0x999999), 0);
    lv_obj_set_style_text_font(root_files_label, &Inter_20, 0);
    lv_obj_align(root_files_label, LV_ALIGN_TOP_MID, 0, 200);

    total_files_label = lv_label_create(scr);
    lv_obj_set_style_text_color(total_files_label, lv_color_hex(0x999999), 0);
    lv_obj_set_style_text_font(total_files_label, &Inter_20, 0);
    lv_obj_align(total_files_label, LV_ALIGN_TOP_MID, 0, 230);



    // Button with dynamic behavior
    mount_btn = lv_btn_create(scr);
    lv_obj_set_size(mount_btn, 200, 50);
    lv_obj_align(mount_btn, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_set_style_border_width(mount_btn, 0, 0);
    lv_obj_set_style_radius(mount_btn, 10, 0);
    lv_obj_add_event_cb(mount_btn, mount_btn_event_handler, LV_EVENT_CLICKED, nullptr);

    mount_btn_label = lv_label_create(mount_btn);
    lv_obj_set_style_text_font(mount_btn_label, &lv_font_montserrat_16, 0);
    lv_obj_center(mount_btn_label);
    
    // Set initial button state (check current USB status)
    usb_connected = isUSBConnected();
    last_usb_state = usb_connected;
    updateMountButtonState();
    
    refreshSDCardInfo();

}

void refreshSDCardInfo() {
    // If SD card is mounted, don't try to access it
    if (sd_card_mounted) {
        lv_label_set_text(status_label, "SD Card in Mount Mode");
        lv_obj_set_style_text_color(status_label, lv_color_hex(COLOR_ORANGE), 0);
        lv_obj_add_flag(storage_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(folders_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(root_files_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(total_files_label, LV_OBJ_FLAG_HIDDEN);

        return;
    }
    
    // Check SD card presence and update button state if needed
    static bool last_sd_present = true;  // Assume SD card is present initially
    bool current_sd_present = false;
    
    SDCardInfo info = getSDCardInfo();
    current_sd_present = info.detected;
    
    // If SD card presence changed, update the button state
    if (current_sd_present != last_sd_present) {
        if (current_sd_present) {
            Serial.println("SD Card Inserted!");
        } else {
            Serial.println("SD Card Removed!");
            // If SD card was removed while mounted, unmount it
            if (sd_card_mounted) {
                sd_card_mounted = false;
                usb_was_connected_before_mount = false;
                Storage::unmount();
                Serial.println("SD Card unmounted due to removal");
            }
        }
        updateMountButtonState();
        last_sd_present = current_sd_present;
    }
    
    if (!info.detected) {
        lv_label_set_text(status_label, "No SD Card Found");
        lv_obj_set_style_text_color(status_label, lv_color_hex(COLOR_RED), 0);
        lv_obj_add_flag(storage_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(folders_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(root_files_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(total_files_label, LV_OBJ_FLAG_HIDDEN);

    } else {
        lv_label_set_text(status_label, "SD Card Detected");
        lv_obj_set_style_text_color(status_label, lv_color_hex(COLOR_GREEN), 0);
        lv_obj_clear_flag(storage_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(folders_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(root_files_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(total_files_label, LV_OBJ_FLAG_HIDDEN);


        uint64_t freeBytes = info.totalBytes - info.usedBytes;
        float freePct = (info.totalBytes > 0) ? ((float)freeBytes / (float)info.totalBytes) * 100.0f : 0.0f;
        
        String storage_text = "Storage: " + formatBytes(info.totalBytes) +
                              " (" + String(freePct, 0) + "% Free)";
        lv_label_set_text(storage_label, storage_text.c_str());
        lv_label_set_text(folders_label, (String("Folders: ") + info.folderCount).c_str());
        lv_label_set_text(root_files_label, (String("Root Files: ") + info.rootFileCount).c_str());
        lv_label_set_text(total_files_label, (String("Total Files: ") + info.totalFileCount).c_str());
    }
}

SDCardInfo getSDCardInfo() {
    SDCardInfo info;
    
    if (!SD_MMC.setPins(SD_PIN_CLK, SD_PIN_CMD, SD_PIN_D0)) {
        info.detected = false;
        return info;
    }
    
    if (!SD_MMC.begin("/sdcard", /*1-bit*/ true)) {
        info.detected = false;
        return info;
    }
    
    if (SD_MMC.cardType() == CARD_NONE) {
        info.detected = false;
        SD_MMC.end();
        return info;
    }
    
    info.detected = true;
    info.totalBytes = SD_MMC.totalBytes();
    info.usedBytes = SD_MMC.usedBytes();
    countFilesAndFolders("/", info, true);
    SD_MMC.end();
    return info;
}

void countFilesAndFolders(const char* path, SDCardInfo& info, bool isRoot) {
    File dir = SD_MMC.open(path);
    if (!dir || !dir.isDirectory()) return;

    File file = dir.openNextFile();
    while (file) {
        if (file.isDirectory()) {
            info.folderCount++;
            String subPath = String(path) + String(file.name()) + "/";
            countFilesAndFolders(subPath.c_str(), info, false);
        } else {
            info.totalFileCount++;
            if (isRoot) info.rootFileCount++;
        }
        file.close();
        file = dir.openNextFile();
    }
    dir.close();
}

String formatBytes(uint64_t bytes) {
    if (bytes >= 1024ULL * 1024ULL * 1024ULL) {
        double gb = (double)bytes / (1024.0 * 1024.0 * 1024.0);
        return String(gb, 1) + " GB";
    } else if (bytes >= 1024ULL * 1024ULL) {
        double mb = (double)bytes / (1024.0 * 1024.0);
        return String(mb, 1) + " MB";
    } else if (bytes >= 1024ULL) {
        double kb = (double)bytes / 1024.0;
        return String(kb, 1) + " KB";
    } else {
        return String(bytes) + " B";
    }
}

// ───────── NeoPixel Control ─────────
void updateNeoPixel(uint32_t color) {
    if (pixels) {
        // Set color with very low brightness (divide RGB values by 16 for 6.25% brightness)
        uint8_t r = (color >> 16) & 0xFF;
        uint8_t g = (color >> 8) & 0xFF;
        uint8_t b = color & 0xFF;
        
        r = r / 16;
        g = g / 16;
        b = b / 16;
        
        // Use RGB color order (Red, Green, Blue) - the Adafruit library handles the GRB conversion
        pixels->setPixelColor(0, pixels->Color(r, g, b));
        pixels->show();
    }
}


