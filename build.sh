#!/bin/sh

docker run --rm -v /home/kevin/programming/atmel_avr/arduino-powerminder:/home/builder/project arduino-cmake /bin/bash -lc "cd /home/builder/project && cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=avr-gcc-toolchain.cmake && make -C build -j$(nproc)"
