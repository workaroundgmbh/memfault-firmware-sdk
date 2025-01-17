# The following lines of boilerplate have to be in your project's CMakeLists
# in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

set(PROJECT_NAME memfault-esp32-demo-app)
set(CMAKE_PROJECT_NAME ${PROJECT_NAME})

get_filename_component(memfault_firmare_sdk_dir ../../../../ ABSOLUTE)
include(${memfault_firmare_sdk_dir}/ports/esp_idf/memfault.cmake)

# NOTE: This include also applies global compiler options, make sure
# this happens first before defining other targets!
# The esp-idf project() macro creates a project_name.elf target:
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(${PROJECT_NAME})

# Add the Memfault Build ID so each build can have a unique version.
set(IDF_PROJECT_EXECUTABLE ${PROJECT_NAME}.elf)
add_custom_command(TARGET ${IDF_PROJECT_EXECUTABLE}
  POST_BUILD
  # Compute and insert the build id
  COMMAND python ${memfault_firmare_sdk_dir}/scripts/fw_build_id.py ${IDF_PROJECT_EXECUTABLE}
  # Save a copy of the ELF that includes the 'log_fmt' section
  COMMAND ${CMAKE_COMMAND} -E copy ${IDF_PROJECT_EXECUTABLE} ${IDF_PROJECT_EXECUTABLE}.memfault
  # Remove the 'log_fmt' compact log section, which confuses elf2image
  COMMAND ${CMAKE_OBJCOPY} --remove-section log_fmt ${IDF_PROJECT_EXECUTABLE}
)

# Add a custom target to swap over the stashed complete elf above after
# elf2image runs
add_custom_target(swap_original_memfault_elf ALL
  COMMAND ${CMAKE_COMMAND} -E rename ${IDF_PROJECT_EXECUTABLE}.memfault ${IDF_PROJECT_EXECUTABLE}
  DEPENDS app
  COMMENT "Copy over original ELF for Memfault uploading"
)
