cmake_minimum_required(VERSION 3.10)
project(ArduinoProject)
#core path
set(ARDUINO_CORE_PATH "~/Library/Arduino15/packages/arduino/hardware/avr/1.8.6/cores/arduino")
include_directories(${ARDUINO_CORE_PATH})

# If you have libraries to include, add their paths as well
set(ARDUINO_LIB_PATH "~/Library/Arduino15/packages/arduino/hardware/avr/1.8.6/libraries")
include_directories(${ARDUINO_LIB_PATH})

# Set the source file (for example, if the main file is `main.cpp`)
set(SOURCE_FILES main.cpp)

# Add an executable target (use this target to work on code, not for direct uploading)
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

# Define a custom command to build and upload using Arduino CLI
add_custom_target(upload
        COMMAND arduino-cli compile -b arduino:avr:uno ${CMAKE_CURRENT_SOURCE_DIR} --output-dir ${CMAKE_CURRENT_BINARY_DIR}/build
        COMMAND arduino-cli upload -b arduino:avr:uno -p /dev/ttyUSB0 --input-dir ${CMAKE_CURRENT_BINARY_DIR}/build
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Compiling and uploading to Arduino"
)
