# Debian cross compiler and QEMU user-mode runner for big-endian tests.
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR s390x)
set(CMAKE_C_COMPILER s390x-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER s390x-linux-gnu-g++)
set(CMAKE_CROSSCOMPILING_EMULATOR qemu-s390x -L /usr/s390x-linux-gnu)
