cmake_minimum_required(VERSION 3.15)
project(ArduinoProject C CXX ASM)

option(PROJECT_WARNINGS_AS_ERRORS "Fail build on warnings for project sources" ON)

# --------------------------------------------------
# MCU & Board settings
# --------------------------------------------------
set(MCU atmega328p)       # Arduino Uno
set(F_CPU 16000000UL)     # 16 MHz
set(BAUD 115200)          # Serial baud rate for avrdude

# --------------------------------------------------
# Arduino Core Paths (adjust if different in container)
# --------------------------------------------------
set(ARDUINO_SDK_PATH /usr/share/arduino)
set(ARDUINO_CORE_PATH ${ARDUINO_SDK_PATH}/hardware/arduino/avr/cores/arduino)
set(ARDUINO_VARIANT_PATH ${ARDUINO_SDK_PATH}/hardware/arduino/avr/variants/standard)
set(ARDUINO_LIB_PATH ${ARDUINO_SDK_PATH}/hardware/arduino/avr/libraries)

# --------------------------------------------------
# Collect sources
# --------------------------------------------------
# Your sources
set(SRC_FILES
    main.cpp
    relay.cpp
    config.cpp
    console.cpp
    console_logic.cpp
)
list(TRANSFORM SRC_FILES PREPEND src/)

# Arduino core sources
file(GLOB ARDUINO_CORE_SRC
    ${ARDUINO_CORE_PATH}/*.c
    ${ARDUINO_CORE_PATH}/*.cpp
    ${ARDUINO_CORE_PATH}/*.S
)

function(add_arduino_core_library target libname)
    set(libpath ${ARDUINO_SDK_PATH}/hardware/arduino/avr/libraries/${libname}/src)

    if(EXISTS ${libpath})
        target_include_directories(${target} PRIVATE ${libpath})
        file(GLOB_RECURSE LIBSRC ${libpath}/*.c ${libpath}/*.cpp)
        target_sources(${target} PRIVATE ${LIBSRC})
    else()
        message(FATAL_ERROR "Core Arduino library '${libname}' not found at ${libpath}")
    endif()
endfunction()

function(add_arduino_user_library target libname basepath)
    if(${ARGC} GREATER 3)
        set(libpath "${basepath}/${libname}/${ARGV3}")
    else()
	set(libpath "${basepath}/${libname}")
    endif()
    if(EXISTS ${libpath})
        target_include_directories(${target} PRIVATE ${libpath})
        file(GLOB_RECURSE LIBSRC ${libpath}/*.c ${libpath}/*.cpp)
        target_sources(${target} PRIVATE ${LIBSRC})
    else()
        message(FATAL_ERROR "User Arduino library '${libname}' not found at ${libpath}")
    endif()
endfunction()

# --------------------------------------------------
# Targets
# --------------------------------------------------
add_executable(${PROJECT_NAME}.elf ${SOURCES}
    ${SRC_FILES}
    ${ARDUINO_CORE_SRC}
    ${WIRE_SRC}
    ${LOCAL_LIB_SRC}
)

# Run avr-size every time the ELF is linked
add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_SIZE} --mcu=${MCU} -C $<TARGET_FILE:${PROJECT_NAME}.elf>
    COMMENT "AVR memory usage"
)

# --------------------------------------------------
# Include paths
# --------------------------------------------------
target_include_directories(${PROJECT_NAME}.elf PRIVATE
    src
    ${ARDUINO_CORE_PATH}
    ${ARDUINO_VARIANT_PATH}
)

# --------------------------------------------------
# Compiler & linker flags
# --------------------------------------------------
target_compile_definitions(${PROJECT_NAME}.elf PRIVATE
    ARDUINO=10819
    ARDUINO_ARCH_AVR
    ARDUINO_AVR_UNO
    F_CPU=${F_CPU}
    __AVR_ATmega328P__
)

target_compile_options(${PROJECT_NAME}.elf PRIVATE
    -mmcu=${MCU}
    $<$<COMPILE_LANGUAGE:C>:-std=gnu11>
    $<$<COMPILE_LANGUAGE:CXX>:-std=gnu++11 -fno-exceptions -fno-threadsafe-statics -fno-rtti>
    -Os
    -Wall
    -ffunction-sections
    -fdata-sections
    -MMD
    -flto
)

if(PROJECT_WARNINGS_AS_ERRORS)
    set_source_files_properties(${SRC_FILES} PROPERTIES COMPILE_OPTIONS "-Werror")
endif()

target_link_options(${PROJECT_NAME}.elf PRIVATE
    -mmcu=${MCU}
    -Wl,--gc-sections
    -Wl,-Map,${PROJECT_NAME}.map
    -flto
)

# This firmware needs only the Arduino core (HardwareSerial, digital I/O).
# No external I2C/LCD/keypad libraries are required.

# Generate HEX file
add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex -R .eeprom ${PROJECT_NAME}.elf ${PROJECT_NAME}.hex
    COMMAND ${CMAKE_SIZE} ${PROJECT_NAME}.elf
)

# Generate an assembly listing (.lss) and a symbol/nm dump after link.
# Use avr-objdump (fallback to 'objdump' if not found). We run via /bin/sh
# so we can redirect output into files.
find_program(AVR_OBJDUMP_EXEC NAMES avr-objdump objdump)
if(NOT AVR_OBJDUMP_EXEC)
    set(AVR_OBJDUMP_EXEC avr-objdump)
endif()

add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    # Use a small helper script to produce the listing and symbol dump so
    # the generated Makefile doesn't end up with wrapped/quoted lines that
    # cause stray leading spaces in filenames.
    COMMAND ${CMAKE_COMMAND} -E env AVR_OBJDUMP_EXEC=${AVR_OBJDUMP_EXEC} ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}
    COMMAND ${CMAKE_COMMAND} -E echo 'Generating assembly listing (.lss) and symbol dump (.sym) from ELF'
    COMMAND ${CMAKE_COMMAND} -E env OBJDUMP=${AVR_OBJDUMP_EXEC} ${CMAKE_SOURCE_DIR}/tools/gen_listing.sh ${AVR_OBJDUMP_EXEC} $<TARGET_FILE:${PROJECT_NAME}.elf> ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.lss ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.sym
    COMMENT "Generating assembly listing (.lss) and symbol dump (.sym) from ELF"
)

# Flash target (change /dev/ttyACM0 if needed)
add_custom_target(flash
    COMMAND avrdude -V -p ${MCU} -c arduino -P /dev/ttyACM0 -b ${BAUD} -U flash:w:${PROJECT_NAME}.hex
    DEPENDS ${PROJECT_NAME}.elf
)
