# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/claudia/esp/esp-idf/components/bootloader/subproject"
  "/home/claudia/esp/Embedded-System-Architectures/build/bootloader"
  "/home/claudia/esp/Embedded-System-Architectures/build/bootloader-prefix"
  "/home/claudia/esp/Embedded-System-Architectures/build/bootloader-prefix/tmp"
  "/home/claudia/esp/Embedded-System-Architectures/build/bootloader-prefix/src/bootloader-stamp"
  "/home/claudia/esp/Embedded-System-Architectures/build/bootloader-prefix/src"
  "/home/claudia/esp/Embedded-System-Architectures/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/claudia/esp/Embedded-System-Architectures/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/claudia/esp/Embedded-System-Architectures/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
