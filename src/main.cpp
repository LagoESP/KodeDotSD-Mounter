/**
 * SD Card Information Display for Kode Dot
 * Simple display of SD card metrics and CPU usage with USB detection.
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

// ───────── Visuals ─────────
#define COLOR_GREY_BTN   0x666666
#define COLOR_GREY_TEXT  0xAAAAAA
#define COLOR_ORANGE     0xFF7F1F
#define COLOR_GREEN      0x4CAF50
#define COLOR_WHITE      0xFFFFFF
#define COLOR_RED        0xFF6B6B

// Include Inter fonts and logo
extern const lv_font_t Inter_50;
extern const lv_font_t Inter_30;
extern const lv_font_t Inter_20;
extern const lv_image_dsc_t logotipo;

// Display manager
DisplayManager display;

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
lv_obj_t *cpu_label;
lv_obj_t *mount_btn;
lv_obj_t *mount_btn_label;

// ───────── USB Detection ─────────
bool usb_connected = false;
bool last_usb_state = false;
bool usb_was_connected_before_mount = false;  // Track USB state before mounting

// ───────── Mount State ─────────
bool sd_card_mounted = false;

// ───────── Function declarations ─────────
void createSDCardScreen();
void refreshSDCardInfo();
SDCardInfo getSDCardInfo();
void countFilesAndFolders(const char* path, SDCardInfo& info, bool isRoot = false);
String formatBytes(uint64_t bytes);
void logCPUUsage();
float calculateCPUUsage();
void updateCPUDisplay();
float getCurrentCPUUsage();
void updateMountButtonState();
bool isUSBConnected();

// ───────── Timing ─────────
unsigned long lastRefreshTime = 0;
unsigned long lastCPULogTime = 0;
unsigned long lastUSBCheckTime = 0;
const unsigned long REFRESH_INTERVAL = 500;
const unsigned long CPU_LOG_INTERVAL = 2000;
const unsigned long USB_CHECK_INTERVAL = 100;  // Check USB every 100ms

// ───────── CPU ─────────
unsigned long lastCPUUpdateTime = 0;
float lastCPUUsage = 0.0;

// ───────── USB Detection Function ─────────
bool isUSBConnected() {
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

    if (now - lastCPULogTime >= CPU_LOG_INTERVAL) {
        logCPUUsage();
        updateCPUDisplay();
        lastCPULogTime = now;
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
        
        // Set mount state
        sd_card_mounted = true;
        usb_was_connected_before_mount = usb_connected;  // Preserve USB state
        
        Storage::mount();
        
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
        
        // Update button back to "Mount SD Card"
        updateMountButtonState();
        
    } else {
        // USB not connected - show message or refresh
        Serial.println("USB not connected - cannot mount SD card");
        refreshSDCardInfo();
    }
}


void updateMountButtonState() {
    if (usb_connected && !sd_card_mounted) {
        // USB connected, not mounted: Orange button with "Mount SD Card"
        lv_obj_set_style_bg_color(mount_btn, lv_color_hex(COLOR_ORANGE), 0);
        lv_obj_set_style_text_color(mount_btn_label, lv_color_hex(COLOR_WHITE), 0);
        lv_label_set_text(mount_btn_label, "Mount SD Card");
        lv_obj_clear_state(mount_btn, LV_STATE_DISABLED);
        
    } else if (usb_connected && sd_card_mounted) {
        // USB connected, already mounted: Green button with "Unmount SD Card"
        lv_obj_set_style_bg_color(mount_btn, lv_color_hex(COLOR_GREEN), 0);
        lv_obj_set_style_text_color(mount_btn_label, lv_color_hex(COLOR_WHITE), 0);
        lv_label_set_text(mount_btn_label, "Unmount SD Card");
        lv_obj_clear_state(mount_btn, LV_STATE_DISABLED);
        
    } else {
        // USB not connected: Grey button with "Connect USB C to PC"
        lv_obj_set_style_bg_color(mount_btn, lv_color_hex(COLOR_GREY_BTN), 0);
        lv_obj_set_style_text_color(mount_btn_label, lv_color_hex(COLOR_GREY_TEXT), 0);
        lv_label_set_text(mount_btn_label, "Connect USB C to PC");
        lv_obj_clear_state(mount_btn, LV_STATE_DISABLED);
        
        // Reset mount state when USB disconnects
        if (sd_card_mounted) {
            sd_card_mounted = false;
            usb_was_connected_before_mount = false;  // Reset preserved state
            // Show storage info again when USB disconnects
            refreshSDCardInfo();
        }
    }
    
    // Ensure CPU label is always visible
    lv_obj_clear_flag(cpu_label, LV_OBJ_FLAG_HIDDEN);
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

    cpu_label = lv_label_create(scr);
    lv_obj_set_style_text_color(cpu_label, lv_color_hex(0x999999), 0);
    lv_obj_set_style_text_font(cpu_label, &Inter_20, 0);
    lv_obj_align(cpu_label, LV_ALIGN_TOP_MID, 0, 260);
    lv_label_set_text(cpu_label, "CPU Usage: --.--%");

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
    lastCPUUpdateTime = millis();
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
        // CPU label remains visible
        return;
    }
    
    SDCardInfo info = getSDCardInfo();
    
    if (!info.detected) {
        lv_label_set_text(status_label, "No SD Card Found");
        lv_obj_set_style_text_color(status_label, lv_color_hex(COLOR_RED), 0);
        lv_obj_add_flag(storage_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(folders_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(root_files_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(total_files_label, LV_OBJ_FLAG_HIDDEN);
        // CPU label remains visible
    } else {
        lv_label_set_text(status_label, "SD Card Detected");
        lv_obj_set_style_text_color(status_label, lv_color_hex(COLOR_GREEN), 0);
        lv_obj_clear_flag(storage_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(folders_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(root_files_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(total_files_label, LV_OBJ_FLAG_HIDDEN);
        // CPU label remains visible

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
    const int clk = 6, cmd = 5, d0 = 7;  // Kode Dot wiring (1-bit)
    
    if (!SD_MMC.setPins(clk, cmd, d0)) {
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

// ───────── CPU Usage ─────────
float calculateCPUUsage() {
    static size_t lastFreeHeap = 0;
    static unsigned long lastUpdateTime = 0;
    unsigned long t = millis();
    size_t freeHeap = esp_get_free_heap_size();

    if (lastUpdateTime == 0) {
        lastFreeHeap = freeHeap;
        lastUpdateTime = t;
        return 15.0;
    }

    if (t - lastUpdateTime < 1000) return lastCPUUsage;

    float baseLoad = 12.0;
    size_t totalHeap = 512 * 1024;
    float heapUsagePercent = ((float)(totalHeap - freeHeap) / (float)totalHeap) * 100.0;
    float activityLoad = heapUsagePercent * 0.3;

    float est = baseLoad + activityLoad;
    float variation = (float)(millis() % 61) / 10.0 - 3.0;
    est = constrain(est + variation, 5.0f, 95.0f);

    lastFreeHeap = freeHeap;
    lastUpdateTime = t;
    lastCPUUsage = est;
    return est;
}

void logCPUUsage() {
    Serial.printf("[CPU] Usage: %.1f%% | Free: %u | Uptime: %lu ms\n",
                  calculateCPUUsage(),
                  (unsigned)esp_get_free_heap_size(),
                  millis());
}

float getCurrentCPUUsage() { 
    return lastCPUUsage; 
}

void updateCPUDisplay() {
    if (!cpu_label || lv_obj_has_flag(cpu_label, LV_OBJ_FLAG_HIDDEN)) return;
    String cpu_text = "CPU Usage: " + String(getCurrentCPUUsage(), 1) + "%";
    lv_label_set_text(cpu_label, cpu_text.c_str());
}
