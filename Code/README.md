Getting portaudio running on Linux:
    run "sudo apt-get install libasound-dev"
    download portaudio .tgz (http://www.portaudio.com/download.html)
    save portaudio folder to local machine
    cd to portaudio folder in terminal
    run "./configure && make"
    run "sudo make install" to create shared library in /usr/local/lib
    create a c program (example below paex_record) that imports portaudio.h
    compile using gcc:
        gcc -L/usr/local/lib -Wl,-rpath=/usr/local/lib -Wall -o paex_record paex_record.c -lportaudio

Adding Kiss-FFT:
    install cmake if necessary ("sudo apt-get -y install cmake")
    navigate to kiss-fft folder in terminal
    install library:
        cmake . -DCMAKE_BUILD_TYPE=RelWithDebInfo
        make
        sudo make install
    compile program using gcc:
        gcc -g -L/usr/local/lib -Wl,-rpath=/usr/local/lib -Wall -o audio_visualizer audio_visualizer.c -lportaudio -lkiss-fft -lrgbmatrix -lm

Compiling rgbmatrix examples:
    run rgb-matrix.sh (use convenience option) to compile everything the first time
    to compile or re-compile a file:
        cc -I../include -Wall -O3 -g -Wextra -Wno-unused-parameter -c -o c-example.o c-example.c
        cc c-example.o -o c-example -L../lib -lrgbmatrix -lrt -lm -lpthread -lstdc++

Putting it all together:
    copy librgbmatrix.a and librgbmatrix.so.1 from rpi-rgb-led-matrix/lib to user/local/lib
    gcc -g -Irpi-rgb-led-matrix/include -L/usr/local/lib -Wl,-rpath=/usr/local/lib -Wall -O3 -Wextra -Wno-unused-parameter -o audio_visualizer audio_visualizer.c -lportaudio -lkiss-fft -lrgbmatrix -lrt -lm -lpthread -lstdc++
     