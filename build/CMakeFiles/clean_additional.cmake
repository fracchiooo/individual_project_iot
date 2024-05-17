# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "")
  file(REMOVE_RECURSE
  "bootloader/bootloader.bin"
  "bootloader/bootloader.elf"
  "bootloader/bootloader.map"
  "config/sdkconfig.cmake"
  "config/sdkconfig.h"
  "esp-idf/esptool_py/flasher_args.json.in"
  "esp-idf/mbedtls/x509_crt_bundle"
  "flash_app_args"
  "flash_bootloader_args"
  "flash_project_args"
  "flasher_args.json"
  "isrgrootx1.pem.S"
  "ldgen_libraries"
  "ldgen_libraries.in"
  "object_project.cert.pem.S"
  "object_project.private.key.S"
  "progetto_singolo.bin"
  "progetto_singolo.map"
  "project_elf_src_esp32s3.c"
  "root_CA.crt.S"
  "x509_crt_bundle.S"
  )
endif()
