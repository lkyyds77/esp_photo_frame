# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "D:/esp-idf/ESP/v5.1.4/esp-idf/components/bootloader/subproject"
  "D:/ESP32-S3/esp_photo_frame/build/bootloader"
  "D:/ESP32-S3/esp_photo_frame/build/bootloader-prefix"
  "D:/ESP32-S3/esp_photo_frame/build/bootloader-prefix/tmp"
  "D:/ESP32-S3/esp_photo_frame/build/bootloader-prefix/src/bootloader-stamp"
  "D:/ESP32-S3/esp_photo_frame/build/bootloader-prefix/src"
  "D:/ESP32-S3/esp_photo_frame/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "D:/ESP32-S3/esp_photo_frame/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "D:/ESP32-S3/esp_photo_frame/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
