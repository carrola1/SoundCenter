# libnfc
## Installation
- Following instructions from Adafruit [Link](https://learn.adafruit.com/adafruit-nfc-rfid-on-raspberry-pi/building-libnfc)
- Download library
    ```
    cd Documents/GitHub/SoundCenter/Code/NFC
    mkdir libnfc
    cd libnfc
    wget https://github.com/nfc-tools/libnfc/releases/download/libnfc-1.7.0/libnfc-1.7.0.tar.bz2
    ```
- Extract tar ball
- Setup config file
    ```
    cd libnfc-1.7.0
    sudo mkdir /etc/nfc
    sudo mkdir /etc/nfc/devices.d
    sudo cp contrib/libnfc/pn532_uart_on_rpi.conf.sample /etc/nfc/devices.d/pn532_uart_on_rpi.conf
    sudo nano /etc/nfc/devices.d/pn532_uart_on_rpi.conf
    ```
        -> add line: allow_intrusive_scan = true
        
- Build/Install
    ```
    sudo make clean
    sudo make install all
    ```
- First time through got errors about libtool version compatibility. Did this to fix:
    ```
    rm -f aclocal.m4
    aclocal && libtoolize --force && autoreconf
    ```
    - then went back to configure package step and repeated everything through Build/Install
***

# I2S Driver
## Installation
- Download/Install
    ```
    curl -sS https://raw.githubusercontent.com/adafruit/Raspberry-Pi-Installer-Scripts/master/i2samp.sh | bash
    ```
    - Select yes for playback in the background at boot
- Re-run the install script again to test the speaker
