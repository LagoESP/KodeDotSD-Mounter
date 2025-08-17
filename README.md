# KodeDotSD-Mounter

A comprehensive SD Card information display application for the Kode Dot ESP32-S3 development board. This project provides real-time monitoring of SD card metrics, CPU usage, and USB connection status with an intuitive touchscreen interface and visual LED feedback.

## 🚀 Features

### 📱 Interactive Touchscreen Interface
- **Real-time SD Card Monitoring**: Display storage capacity, used/free space, and file statistics
- **File System Analysis**: Count total files, folders, and root-level files
- **Dynamic Status Updates**: Live refresh of SD card information every 500ms
- **Professional UI**: Clean, modern interface using LVGL graphics library

### 🔌 USB Mass Storage Integration
- **Smart USB Detection**: Automatically detects when USB is connected to a computer
- **SD Card Mounting**: One-touch mounting/unmounting of SD card for file transfer
- **State Preservation**: Maintains USB connection state during mount operations
- **Safe Unmounting**: Proper cleanup when disconnecting USB
- **Smart Button States**: Button automatically disables when no SD card is detected

### 💡 Visual Status Indicators
- **NeoPixel LED Feedback**: RGB LED that changes color based on system status
  - 🟠 **Orange**: USB connected, ready to mount SD card
  - 🟢 **Green**: SD card mounted and accessible via USB
  - ⚫ **Off**: No USB connection
- **Low Power Mode**: LED operates at 6.25% brightness for comfortable viewing
- **Real-time Updates**: LED color changes instantly with system state changes

### 📊 System Monitoring
- **CPU Usage Tracking**: Real-time CPU utilization monitoring with simulated load calculation
- **Memory Management**: Heap usage monitoring and optimization
- **Performance Logging**: Serial output for debugging and performance analysis

### 🎨 Customizable Interface
- **Inter Font Family**: Professional typography with multiple font sizes (20, 30, 50)
- **Dynamic Color Schemes**: Context-aware color changes for different states
- **Responsive Layout**: Optimized for Kode Dot's 410x502 pixel display

## 🛠️ Hardware Requirements

- **Kode Dot ESP32-S3 Development Board**
- **MicroSD Card** (any capacity)
- **USB-C Cable** for connection to computer
- **Display**: Built-in 410x502 pixel LCD touchscreen
- **NeoPixel**: Built-in RGB LED indicator

## 📋 Prerequisites

- **PlatformIO** IDE or CLI
- **Arduino Framework** support
- **ESP32-S3 Development Tools**

## 🔧 Installation & Setup

### 1. Clone the Repository
```bash
git clone https://github.com/LagoESP/KodeDotSD-Mounter.git
cd KodeDotSD-Mounter
```

### 2. Install Dependencies
The project automatically installs required libraries via PlatformIO:
- `lvgl/lvgl` - Graphics library
- `adafruit/Adafruit NeoPixel` - LED control
- `moononournation/GFX Library` - Graphics support
- Additional Kode Dot specific libraries

### 3. Build and Upload
```bash
# Build the project
pio run

# Upload to your Kode Dot board
pio run --target upload

# Monitor serial output
pio device monitor
```

## 📖 Usage Guide

### Initial Setup
1. **Power On**: Connect your Kode Dot board via USB
2. **Insert SD Card**: Place a microSD card in the slot
3. **Boot**: The system will automatically detect the SD card and display information

### Using the Interface

#### 📊 SD Card Information Display
- **Storage Capacity**: Shows total space and free space percentage
- **File Counts**: Displays total files, folders, and root-level files
- **Real-time Updates**: Information refreshes automatically every 500ms

#### 🔌 USB Mass Storage Mode
1. **Connect USB**: Plug USB-C cable into your computer
2. **Check Button State**: 
   - **Orange "Mount SD Card"**: SD card detected, ready to mount
   - **Grey "No SD Card"**: No SD card detected (button disabled)
   - **Grey "Connect USB C to PC"**: No USB connection
3. **Mount SD Card**: Press the orange "Mount SD Card" button
4. **File Transfer**: Access the SD card as a removable drive on your computer
5. **Unmount**: Press the green "Unmount SD Card" button when done
6. **Safe Removal**: Unplug USB cable

#### 💡 LED Status Guide
- **Orange LED**: Ready to mount SD card (USB connected, SD card detected)
- **Green LED**: SD card mounted and accessible
- **No LED**: No USB connection or no SD card detected

### 🔍 Monitoring and Debugging
- **Serial Monitor**: Connect at 115200 baud for detailed logging
- **CPU Usage**: Real-time CPU utilization display
- **Memory Status**: Heap usage and performance metrics

## 🏗️ Project Structure

```
KodeDotSD-Mounter/
├── src/                    # Main source code
│   ├── main.cpp           # Primary application logic
│   ├── fonts/             # Inter font family files
│   └── images/            # Logo and image assets
├── lib/                    # Custom libraries
│   └── kodedot_bsp/       # Kode Dot board support package
├── boards/                 # Board configuration files
├── platformio.ini         # PlatformIO configuration
└── partitions_app.csv     # ESP32 partition layout
```

## 🔧 Configuration

### Pin Configuration
All pins are automatically configured via `kodedot/pin_config.h`:
- **SD Card**: Pins 5, 6, 7 (CMD, CLK, D0)
- **NeoPixel**: Pin 4
- **Display**: SPI pins 17, 15, 14, 16, 10, 8, 9

### Customization Options
- **Refresh Intervals**: Modify timing constants in the code
- **Color Schemes**: Update color definitions for different themes
- **Font Sizes**: Adjust text sizing for different display preferences

## 🐛 Troubleshooting

### Common Issues

#### SD Card Not Detected
- Ensure SD card is properly inserted
- Check if SD card is formatted (FAT32 recommended)
- Verify SD card is not corrupted

#### USB Connection Issues
- Try different USB-C cable
- Check USB port compatibility
- Ensure proper drivers are installed

#### Display Problems
- Verify LVGL configuration
- Check font files are properly included
- Monitor serial output for error messages

### Debug Information
Enable serial monitoring to see:
- USB connection status
- SD card detection results
- Memory usage statistics
- Error messages and warnings

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- **Kode Dot Team** for the excellent development board
- **LVGL Community** for the powerful graphics library
- **Adafruit** for the NeoPixel library
- **ESP32 Community** for the robust platform support

## 📞 Support

- **Issues**: Report bugs and feature requests via GitHub Issues
- **Discussions**: Join community discussions in GitHub Discussions
- **Documentation**: Visit [docs.kode.diy](https://docs.kode.diy) for Kode Dot resources

---

**Made with ❤️ for the Kode Dot community**
