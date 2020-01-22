#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! Dependency functions required in order to use the Memfault coredump API
//!
//! @note The expectation is that all these functions are safe to call from ISRs and with
//!   interrupts disabled.
//! @note It's also expected the caller has implemented memfault_platform_get_device_info(). That way
//!   we can save off information that allows for the coredump to be further decoded server side

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>

#include "memfault/panics/trace_reason_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum MfltCoredumpRegionType {
  kMfltCoredumpRegionType_Memory,
  kMfltCoredumpRegionType_MemoryWordAccessOnly,
  kMfltCoredumpRegionType_ImageIdentifier,
} eMfltCoredumpRegionType;

//! Convenience macro to define a sMfltCoredumpRegion of type kMfltCoredumpRegionType_Memory.
#define MEMFAULT_COREDUMP_MEMORY_REGION_INIT(_start, _size) \
  (sMfltCoredumpRegion) { \
    .type = kMfltCoredumpRegionType_Memory, \
    .region_start = _start, \
    .region_size = _size, \
  }

typedef struct MfltCoredumpRegion {
  eMfltCoredumpRegionType type;
  const void *region_start;
  uint32_t region_size;
} sMfltCoredumpRegion;

typedef struct CoredumpCrashInfo {
  //! The address of the stack at the time of the error. This makes it easy to generate
  //! special regions such as "just the top of the stack"
  void *stack_address;
  //! The reason the reset is taking place. Sometimes you may want to collect different memory
  //! regions based on the reset type (i.e assert vs HardFault)
  eMfltResetReason trace_reason;
} sCoredumpCrashInfo;

//! Returns an array of the regions to capture when the system crashes
//! @param crash_info Information pertaining to the crash. The user of the SDK can decide
//!   whether or not to use this info when generating coredump regions to collect. Some
//!   example ideas can be found in the comments for the struct above
//! @param num_regions The number of regions in the list returned
const sMfltCoredumpRegion *memfault_platform_coredump_get_regions(
    const sCoredumpCrashInfo *crash_info, size_t *num_regions);

typedef struct MfltCoredumpStorageInfo {
  //! The size of the coredump storage region (must be greater than the space needed to capture all
  //! the regions returned from @ref memfault_platform_coredump_get_regions)
  size_t size;
  //! Sector size for storage medium used for coredump storage
  size_t sector_size;
} sMfltCoredumpStorageInfo;

//! Return info pertaining to the region a coredump will be stored in
void memfault_platform_coredump_storage_get_info(sMfltCoredumpStorageInfo *info);

//! Issue write to the platforms coredump storage region
//!
//! @param offset the offset within the platform coredump storage region to write to
//! @param data opaque data to write
//! @param data_len length of data to write in bytes
bool memfault_platform_coredump_storage_write(uint32_t offset, const void *data,
                                              size_t data_len);

//! Read from platforms coredump storage region
//!
//! @param offset the offset within the platform coredump storage region to read from
//! @param data the buffer to read the data into
//! @param read_len length of data to read in bytes
bool memfault_platform_coredump_storage_read(uint32_t offset, void *data,
                                             size_t read_len);

//! Erase a region of the platforms coredump storage
//!
//! @param offset to start erasing within the coredump storage region (must be sector_size
//!   aligned)
//! @param erase_size The amount of bytes to erase (must be in sector_size units)
bool memfault_platform_coredump_storage_erase(uint32_t offset, size_t erase_size);

//! Invalidate any saved coredumps within the platform storage coredump region
//!
//! Coredumps are persisted until this function is called so this is useful to call after a
//! coredump has been read so a new coredump will be captured upon next crash
//!
//! @note a coredump region can be invalidated by zero'ing out or erasing the first sector
//! being used for storage
void memfault_platform_coredump_storage_clear(void);

//! Used to read coredumps out of storage when the system is not in a _crashed_ state
//!
//! @note A weak version of this API is defined in memfault_coredump.c and it will just use the
//! implementation defined in memfault_platform_coredump_storage_read(). If the storage read implementation
//! differs between when a coredump is saved and when it is read (i.e needs mutex locking), you can override the
//! the function by defining it in your port file.
//!
//! @param offset The offset to start reading at
//! @param buf The buffer to copy data to
//! @param buf_len The number of bytes to read
//!
//! @return true if the read was successful, false otherwise
extern bool memfault_coredump_read(uint32_t offset, void *buf, size_t buf_len);

#ifdef __cplusplus
}
#endif
