//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! A minimal implementation of a CBOR encoder. See header for more details

#include "memfault/util/cbor.h"

#include <stdio.h>
#include <string.h>

// https://tools.ietf.org/html/rfc7049#section-2.1
typedef enum CborMajorType {
  kCborMajorType_UnsignedInteger = 0,
  kCborMajorType_NegativeInteger = 1,
  kCborMajorType_ByteString = 2,
  kCborMajorType_TextString = 3,
  kCborMajorType_Array = 4,
  kCborMajorType_Map = 5,
  kCborMajorType_Tag = 6,
  kCborMajorType_SimpleType = 7,
} eCborMajorType;

// A CBOR payload is composed of a stream of "data items"
// The main type of each "data item" is known as the CBOR Major Type
// and populates the upper 3 bits of the first byte of each "data item"
#define CBOR_SERIALIZE_MAJOR_TYPE(mt) ((uint8_t) ((mt & 0x7) << 5))

void memfault_cbor_encoder_init(sMemfaultCborEncoder *encoder, MemfaultCborWriteCallback write_cb,
                                void *write_cb_ctx, size_t buf_len) {
  const bool compute_size_only = (write_cb == NULL);
  *encoder = (sMemfaultCborEncoder) {
    .compute_size_only = compute_size_only,
    .write_cb = write_cb,
    .write_cb_ctx = write_cb_ctx,
    .buf_len = buf_len,
  };
}

void memfault_cbor_encoder_size_only_init(sMemfaultCborEncoder *encoder) {
  memfault_cbor_encoder_init(encoder, NULL, NULL, 0);
}

size_t memfault_cbor_encoder_deinit(sMemfaultCborEncoder *encoder) {
  const size_t bytes_encoded = encoder->encoded_size;
  *encoder = (sMemfaultCborEncoder) { 0 };
  return bytes_encoded;
}

static bool prv_add_to_result_buffer(sMemfaultCborEncoder *encoder, const void *data,
                                     size_t data_len) {
  if (data_len == 0) {
    // no work to do
    return true;
  }

  if (encoder->compute_size_only) {
    // no need to perform size checks because we are only computing the total encoder size
    encoder->encoded_size += data_len;
    return true;
  }

  if ((encoder->encoded_size + data_len) > encoder->buf_len) {
    // not enough space
    return false;
  }
  encoder->write_cb(encoder->write_cb_ctx, encoder->encoded_size, data, data_len);
  encoder->encoded_size += data_len;
  return true;
}

static bool prv_encode_unsigned_integer(
    sMemfaultCborEncoder *encoder, uint8_t major_type, uint32_t val) {
  uint8_t mt = CBOR_SERIALIZE_MAJOR_TYPE(major_type);

  uint8_t tmp_buf[5];
  uint8_t *p = &tmp_buf[0];

  if (val < 24) {
    *p++ = mt + (uint8_t)val;
  } else {
    if (val <= UINT8_MAX) {
      *p++ = mt + 24;
      *p++ = val & 0xff;
    } else if (val <= UINT16_MAX) {
      *p++ = mt + 25;
      *p++ = (val >> 8) & 0xff;
      *p++ = val & 0xff;
    } else {
      *p++ = mt + 26;
      *p++ = (val >> 24) & 0xff;
      *p++ = (val >> 16) & 0xff;
      *p++ = (val >> 8) & 0xff;
      *p++ = val & 0xff;
    }
  }
  const size_t tmp_buf_len = (uint32_t)(p - tmp_buf);
  return prv_add_to_result_buffer(encoder, tmp_buf, tmp_buf_len);
}

bool memfault_cbor_encode_unsigned_integer(
    sMemfaultCborEncoder *encoder, uint32_t value) {
  return prv_encode_unsigned_integer(encoder, kCborMajorType_UnsignedInteger, value);
}

bool memfault_cbor_encode_signed_integer(sMemfaultCborEncoder *encoder, int32_t value) {
  // Logic derived from "Appendix C Pseudocode" of RFC
  int32_t ui = (value >> 31);
  // Figure out if we are encoding a Negative or Unsigned Integer by reading a sign extension bit
  const uint8_t cbor_major_type = ui & 0x1;
  ui ^= value;
  return prv_encode_unsigned_integer(encoder, cbor_major_type, (uint32_t)ui);
}

bool memfault_cbor_encode_byte_string(sMemfaultCborEncoder *encoder, const void *buf,
                                     size_t buf_len) {
  return (prv_encode_unsigned_integer(encoder, kCborMajorType_ByteString, buf_len) &&
          prv_add_to_result_buffer(encoder, buf, buf_len));
}

bool memfault_cbor_encode_string(sMemfaultCborEncoder *encoder, const char *str) {
  const size_t str_len = strlen(str);
  return (prv_encode_unsigned_integer(encoder, kCborMajorType_TextString,  str_len) &&
          prv_add_to_result_buffer(encoder, str, str_len));
}

bool memfault_cbor_encode_dictionary_begin(
    sMemfaultCborEncoder *encoder, size_t num_elements) {
  return prv_encode_unsigned_integer(encoder, kCborMajorType_Map, num_elements);
}
