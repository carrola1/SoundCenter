# PortAudio
## Installation
- Install libasound
    ```
    sudo apt-get install libasound-dev
    ```
- Download PortAudio .tgz [Link](http://www.portaudio.com/download.html)
- Save portaudio folder to local machine
- cd to portaudio folder in terminal
- Configure and make
    ```
    ./configure && make
    ```
- Install to create shared library in /usr/local/lib
    ```
    sudo make install
    ```

## Compile
- Create a c program (example below paex_record) that imports portaudio.h
- Compile using gcc
    ```
    gcc -L/usr/local/lib -Wl,-rpath=/usr/local/lib -Wall -o paex_record paex_record.c -lportaudio
    ```
***

# Kiss-FFT
## Installation
- Install cmake if necessary
    ```
    sudo apt-get -y install cmake
    ```
- Navigate to kiss-fft folder in terminal
- Install library
    ```
    cmake . -DCMAKE_BUILD_TYPE=RelWithDebInfo
    make
    sudo make install
    ```

## Compile
- Compile using gcc
    ```
    gcc -g -L/usr/local/lib -Wl,-rpath=/usr/local/lib -Wall -o audio_visualizer audio_visualizer.c -lportaudio -lkiss-fft -lrgbmatrix -lm
    ```
***

# RGBMatrix
## Installation
- Follow instruction from Adafruit [Link](https://learn.adafruit.com/adafruit-rgb-matrix-bonnet-for-raspberry-pi/driving-matrices)
    - Use convenience option
    - You can rerun rgb-matrix.sh to compile everything again
- copy librgbmatrix.a and librgbmatrix.so.1 from rpi-rgb-led-matrix/lib to user/local/lib
***
## Compile
- To compile:
    ```
    cc -I../include -Wall -O3 -g -Wextra -Wno-unused-parameter -c -o c-example.o c-example.c
    cc c-example.o -o c-example -L../lib -lrgbmatrix -lrt -lm -lpthread -lstdc++
    ```

# Raspberry Pi GPIO
## Installation
- Dowload pigpio
    ```
    wget https://github.com/joan2937/pigpio/archive/v74.zip
    unzip v74.zip
    ```
- Install
    ```
    cd pigpio-74
    make
    sudo make install
    ```
***

# Putting everything together
## Compile
- Compile command
    ```
    gcc -g -Irpi-rgb-led-matrix/include -L/usr/local/lib -Wl,-rpath=/usr/local/lib -Wall -O3 -Wextra -Wno-unused-parameter -o audio_visualizer audio_visualizer.c -lportaudio -lkiss-fft -lrgbmatrix -lpigpio -lrt -lm -lpthread -lstdc++
    ```
***

# Running at startup
- Create and edit a service file
    ```
    sudo nano /lib/systemd/system/audio_visualizer.service
    ```
- Paste this into service file:
    ```
    [Unit]
    Description=Run Audio Visualizer App
    After=network-online.target
    
    [Service]
    ExecStart=/home/pi/Documents/GitHub/SoundCenter/Code/audio_visualizer
    
    [Install]
    WantedBy=multi-user.target
    ```
- Run the following:
    ```
    sudo systemctl daemon-reload
    sudo systemctl enable audio_visualizer.service
    ```