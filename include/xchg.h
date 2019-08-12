/* xchg.h
 * Copyright (c) 2019 Alex Forster
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/// Represents the set of possible datatypes of values in an <tt>xchg_message</tt> stream.
///
enum xchg_type
{
    xchg_type_invalid,  ///< Indicates an invalid <tt>xchg_type</tt>
    xchg_type_bool,  ///< 1-byte boolean
    xchg_type_int8,  ///< 1-byte signed integer
    xchg_type_uint8,  ///< 1-byte unsigned integer
    xchg_type_int16,  ///< 2-byte signed integer
    xchg_type_uint16,  ///< 2-byte unsigned integer
    xchg_type_int32,  ///< 4-byte signed integer
    xchg_type_uint32,  ///< 4-byte unsigned integer
    xchg_type_int64,  ///< 8-byte signed integer
    xchg_type_uint64,  ///< 8-byte unsigned integer
    xchg_type_float32,  ///< 4-byte floating-point number
    xchg_type_float64,  ///< 8-byte floating-point number
};

/// Represents a stream of values that can be encoded and decoded using the <tt>xchg_message_*</tt> family of functions.
///
/// @note
///   This type should be treated as opaque. Use the various <tt>xchg_message_*</tt> accessor functions to manipulate messages.
///
struct xchg_message
{
    char *data;  ///< @private
    size_t length;  ///< @private
    size_t position;  ///< @private
    char *error;  ///< @private
};

/// Resets <tt>message</tt> and configures it to use the underlying buffer <tt>data</tt> of size <tt>sz_data</tt>.
///
/// @param [in] message
///   pointer to an <tt>xchg_message</tt> structure
/// @param [in] data
///   pointer to a memory buffer
/// @param [in] sz_data
///   size of the <tt>data</tt> memory buffer
/// @return
///   <tt>true</tt> if the <tt>xchg_message</tt> was initialized, or <tt>false</tt> if invalid arguments were provided
/// @note
///   Do not initialize an <tt>xchg_message</tt> that you plan to use with an <tt>xchg_channel</tt>. Instead,
///   use <tt>xchg_channel_prepare</tt> or <tt>xchg_channel_receive</tt> to initialize the <tt>xchg_message</tt>
///   with memory that is backed by the channel's underlying ingress/egress buffers.
/// @memberof xchg_message
///
bool xchg_message_init(struct xchg_message *message, char *data, size_t sz_data);

/// Seeks to the beginning of the underlying buffer of <tt>message</tt> and resets its error state.
///
/// @param [in] message
///   pointer to an <tt>xchg_message</tt> structure
/// @return
///   <tt>true</tt> if the <tt>xchg_message</tt> was reset, or <tt>false</tt> if invalid arguments were provided
/// @memberof xchg_message
///
bool xchg_message_reset(struct xchg_message *message);

/// Provides the current position in the underlying buffer of <tt>message</tt>.
///
/// @param [in] message
///   pointer to an <tt>xchg_message</tt> structure
/// @param [out] position
///   pointer to storage for the current position in the underlying buffer of <tt>message</tt>
/// @return
///   <tt>true</tt> if the position argument was filled, or <tt>false</tt> if invalid arguments were provided
/// @memberof xchg_message
///
bool xchg_message_position(struct xchg_message *message, size_t *position);

/// Seeks to <tt>position</tt> in the underlying buffer of <tt>message</tt>.
///
/// @param [in] message
///   pointer to an <tt>xchg_message</tt> structure
/// @param [in] position
///   absolute position to seek to in the underlying buffer of <tt>message</tt>
/// @return
///   <tt>true</tt> if the underlying buffer's position was updated, or <tt>false</tt> if the requested position is
///   out of bounds
/// @memberof xchg_message
///
bool xchg_message_seek(struct xchg_message *message, size_t position);

/// Provides information about the next value in <tt>message</tt>, without advancing its underlying buffer position.
///
/// @param [in] message
///   pointer to an <tt>xchg_message</tt> structure
/// @param [out] type
///   optional pointer to storage for an <tt>xchg_type</tt> which will indicate the datatype of the next value or list
/// @param [out] null
///   optional pointer to storage for a <tt>bool</tt> which will indicate whether the next value is null
/// @param [out] list
///   optional pointer to storage for a <tt>bool</tt> which will indicate whether the next value is a list
/// @param [out] sz_list
///   optional pointer to storage for a <tt>uint64_t</tt> which will indicate the number of entries in the <tt>list</tt>
/// @return
///   <tt>true</tt> if a valid next value is available in <tt>message</tt>, otherwise <tt>false</tt>
/// @note
///   If this function succeeds, then calling the corresponding <tt>xchg_read_*</tt> function is guaranteed to succeed.
/// @memberof xchg_message
///
bool xchg_message_peek(struct xchg_message *message, enum xchg_type *type, bool *null, bool *list, uint64_t *sz_list);

/// Provides a static string describing the error that occurred during the last operation on <tt>message</tt>.
///
/// @param [in] message
///   pointer to an <tt>xchg_message</tt> structure
/// @return
///   pointer to a static string describing the error that occurred during the last operation on <tt>message</tt>
/// @memberof xchg_message
///
const char *xchg_message_strerror(const struct xchg_message *message);

/// Reads a null value from <tt>message</tt> and advances the underlying buffer position.
///
/// @param [in] message
///   pointer to an <tt>xchg_message</tt> structure
/// @param [out] type
///   pointer to storage for an <tt>xchg_type</tt> which will indicate the datatype of the null value
/// @return
///   <tt>true</tt> if a null value was read, otherwise <tt>false</tt>
/// @memberof xchg_message
///
bool xchg_message_read_null(struct xchg_message *message, enum xchg_type *type);

/// Reads a null list from <tt>message</tt> and advances the underlying buffer position.
///
/// @param [in] message
///   pointer to an <tt>xchg_message</tt> structure
/// @param [out] type
///   pointer to storage for an <tt>xchg_type</tt> which will indicate the datatype of entries in the null list
/// @return
///   <tt>true</tt> if a null list was read, otherwise <tt>false</tt>
/// @memberof xchg_message
///
bool xchg_message_read_null_list(struct xchg_message *message, enum xchg_type *type);

/// Reads a <tt>bool</tt> from <tt>message</tt> and advances the underlying buffer position.
///
/// @param [in] message
///   pointer to an <tt>xchg_message</tt> structure
/// @param [out] value
///   pointer to storage for a <tt>bool</tt> which will hold the value
/// @return
///   <tt>true</tt> if a <tt>bool</tt> was read, otherwise <tt>false</tt>
/// @memberof xchg_message
///
bool xchg_message_read_bool(struct xchg_message *message, bool *value);

/// Reads a list of <tt>bool</tt> from <tt>message</tt> and advances the underlying buffer position.
///
/// @param [in] message
///   pointer to an <tt>xchg_message</tt> structure
/// @param [out] list
///   pointer to storage for a <tt>bool[]</tt> which will contain the list
/// @param [out] sz_list
///   pointer to storage for a <tt>uint64_t</tt> which will indicate the number of entries in <tt>list</tt>
/// @return
///   <tt>true</tt> if a list of <tt>bool</tt> was read, otherwise <tt>false</tt>
/// @memberof xchg_message
///
bool xchg_message_read_bool_list(struct xchg_message *message, bool const *list[], uint64_t *sz_list);

/// Reads an <tt>int8_t</tt> from <tt>message</tt> and advances the underlying buffer position.
///
/// @param [in] message
///   pointer to an <tt>xchg_message</tt> structure
/// @param [out] value
///   pointer to storage for an <tt>int8_t</tt> which will hold the value
/// @return
///   <tt>true</tt> if an <tt>int8_t</tt> was read, otherwise <tt>false</tt>
/// @memberof xchg_message
///
bool xchg_message_read_int8(struct xchg_message *message, int8_t *value);

/// Reads a list of <tt>int8_t</tt> from <tt>message</tt> and advances the underlying buffer position.
///
/// @param [in] message
///   pointer to an <tt>xchg_message</tt> structure
/// @param [out] list
///   pointer to storage for an <tt>int8_t[]</tt> which will contain the list
/// @param [out] sz_list
///   pointer to storage for a <tt>uint64_t</tt> which will indicate the number of entries in <tt>list</tt>
/// @return
///   <tt>true</tt> if a list of <tt>int8_t</tt> was read, otherwise <tt>false</tt>
/// @memberof xchg_message
///
bool xchg_message_read_int8_list(struct xchg_message *message, int8_t const *list[], uint64_t *sz_list);

/// Reads a <tt>uint8_t</tt> from <tt>message</tt> and advances the underlying buffer position.
///
/// @param [in] message
///   pointer to an <tt>xchg_message</tt> structure
/// @param [out] value
///   pointer to storage for a <tt>uint8_t</tt> which will hold the value
/// @return
///   <tt>true</tt> if a <tt>uint8_t</tt> was read, otherwise <tt>false</tt>
/// @memberof xchg_message
///
bool xchg_message_read_uint8(struct xchg_message *message, uint8_t *value);

/// Reads a list of <tt>uint8_t</tt> from <tt>message</tt> and advances the underlying buffer position.
///
/// @param [in] message
///   pointer to an <tt>xchg_message</tt> structure
/// @param [out] list
///   pointer to storage for a <tt>uint8_t[]</tt> which will contain the list
/// @param [out] sz_list
///   pointer to storage for a <tt>uint64_t</tt> which will indicate the number of entries in <tt>list</tt>
/// @return
///   <tt>true</tt> if a list of <tt>uint8_t</tt> was read, otherwise <tt>false</tt>
/// @memberof xchg_message
///
bool xchg_message_read_uint8_list(struct xchg_message *message, uint8_t const *list[], uint64_t *sz_list);

/// Reads an <tt>int16_t</tt> from <tt>message</tt> and advances the underlying buffer position.
///
/// @param [in] message
///   pointer to an <tt>xchg_message</tt> structure
/// @param [out] value
///   pointer to storage for an <tt>int16_t</tt> which will hold the value
/// @return
///   <tt>true</tt> if an <tt>int16_t</tt> was read, otherwise <tt>false</tt>
/// @memberof xchg_message
///
bool xchg_message_read_int16(struct xchg_message *message, int16_t *value);

/// Reads a list of <tt>int16_t</tt> from <tt>message</tt> and advances the underlying buffer position.
///
/// @param [in] message
///   pointer to an <tt>xchg_message</tt> structure
/// @param [out] list
///   pointer to storage for an <tt>int16_t[]</tt> which will contain the list
/// @param [out] sz_list
///   pointer to storage for a <tt>uint64_t</tt> which will indicate the number of entries in <tt>list</tt>
/// @return
///   <tt>true</tt> if a list of <tt>int16_t</tt> was read, otherwise <tt>false</tt>
/// @memberof xchg_message
///
bool xchg_message_read_int16_list(struct xchg_message *message, int16_t const *list[], uint64_t *sz_list);

/// Reads a <tt>uint16_t</tt> from <tt>message</tt> and advances the underlying buffer position.
///
/// @param [in] message
///   pointer to an <tt>xchg_message</tt> structure
/// @param [out] value
///   pointer to storage for a <tt>uint16_t</tt> which will hold the value
/// @return
///   <tt>true</tt> if a <tt>uint16_t</tt> was read, otherwise <tt>false</tt>
/// @memberof xchg_message
///
bool xchg_message_read_uint16(struct xchg_message *message, uint16_t *value);

/// Reads a list of <tt>uint16_t</tt> from <tt>message</tt> and advances the underlying buffer position.
///
/// @param [in] message
///   pointer to an <tt>xchg_message</tt> structure
/// @param [out] list
///   pointer to storage for a <tt>uint16_t[]</tt> which will contain the list
/// @param [out] sz_list
///   pointer to storage for a <tt>uint64_t</tt> which will indicate the number of entries in <tt>list</tt>
/// @return
///   <tt>true</tt> if a list of <tt>uint16_t</tt> was read, otherwise <tt>false</tt>
/// @memberof xchg_message
///
bool xchg_message_read_uint16_list(struct xchg_message *message, uint16_t const *list[], uint64_t *sz_list);

/// Reads an <tt>int32_t</tt> from <tt>message</tt> and advances the underlying buffer position.
///
/// @param [in] message
///   pointer to an <tt>xchg_message</tt> structure
/// @param [out] value
///   pointer to storage for an <tt>int32_t</tt> which will hold the value
/// @return
///   <tt>true</tt> if an <tt>int32_t</tt> was read, otherwise <tt>false</tt>
/// @memberof xchg_message
///
bool xchg_message_read_int32(struct xchg_message *message, int32_t *value);

/// Reads a list of <tt>int32_t</tt> from <tt>message</tt> and advances the underlying buffer position.
///
/// @param [in] message
///   pointer to an <tt>xchg_message</tt> structure
/// @param [out] list
///   pointer to storage for an <tt>int32_t[]</tt> which will contain the list
/// @param [out] sz_list
///   pointer to storage for a <tt>uint64_t</tt> which will indicate the number of entries in <tt>list</tt>
/// @return
///   <tt>true</tt> if a list of <tt>int32_t</tt> was read, otherwise <tt>false</tt>
/// @memberof xchg_message
///
bool xchg_message_read_int32_list(struct xchg_message *message, int32_t const *list[], uint64_t *sz_list);

/// Reads a <tt>uint32_t</tt> from <tt>message</tt> and advances the underlying buffer position.
///
/// @param [in] message
///   pointer to an <tt>xchg_message</tt> structure
/// @param [out] value
///   pointer to storage for a <tt>uint32_t</tt> which will hold the value
/// @return
///   <tt>true</tt> if a <tt>uint32_t</tt> was read, otherwise <tt>false</tt>
/// @memberof xchg_message
///
bool xchg_message_read_uint32(struct xchg_message *message, uint32_t *value);

/// Reads a list of <tt>uint32_t</tt> from <tt>message</tt> and advances the underlying buffer position.
///
/// @param [in] message
///   pointer to an <tt>xchg_message</tt> structure
/// @param [out] list
///   pointer to storage for a <tt>uint32_t[]</tt> which will contain the list
/// @param [out] sz_list
///   pointer to storage for a <tt>uint32_t</tt> which will indicate the number of entries in <tt>list</tt>
/// @return
///   <tt>true</tt> if a list of <tt>uint32_t</tt> was read, otherwise <tt>false</tt>
/// @memberof xchg_message
///
bool xchg_message_read_uint32_list(struct xchg_message *message, uint32_t const *list[], uint64_t *sz_list);

/// Reads an <tt>int64_t</tt> from <tt>message</tt> and advances the underlying buffer position.
///
/// @param [in] message
///   pointer to an <tt>xchg_message</tt> structure
/// @param [out] value
///   pointer to storage for an <tt>int64_t</tt> which will hold the value
/// @return
///   <tt>true</tt> if an <tt>int64_t</tt> was read, otherwise <tt>false</tt>
/// @memberof xchg_message
///
bool xchg_message_read_int64(struct xchg_message *message, int64_t *value);

/// Reads a list of <tt>int64_t</tt> from <tt>message</tt> and advances the underlying buffer position.
///
/// @param [in] message
///   pointer to an <tt>xchg_message</tt> structure
/// @param [out] list
///   pointer to storage for an <tt>int64_t[]</tt> which will contain the list
/// @param [out] sz_list
///   pointer to storage for a <tt>uint64_t</tt> which will indicate the number of entries in <tt>list</tt>
/// @return
///   <tt>true</tt> if a list of <tt>int64_t</tt> was read, otherwise <tt>false</tt>
/// @memberof xchg_message
///
bool xchg_message_read_int64_list(struct xchg_message *message, int64_t const *list[], uint64_t *sz_list);

/// Reads a <tt>uint64_t</tt> from <tt>message</tt> and advances the underlying buffer position.
///
/// @param [in] message
///   pointer to an <tt>xchg_message</tt> structure
/// @param [out] value
///   pointer to storage for a <tt>uint64_t</tt> which will hold the value
/// @return
///   <tt>true</tt> if a <tt>uint64_t</tt> was read, otherwise <tt>false</tt>
/// @memberof xchg_message
///
bool xchg_message_read_uint64(struct xchg_message *message, uint64_t *value);

/// Reads a list of <tt>uint64_t</tt> from <tt>message</tt> and advances the underlying buffer position.
///
/// @param [in] message
///   pointer to an <tt>xchg_message</tt> structure
/// @param [out] list
///   pointer to storage for a <tt>uint64_t[]</tt> which will contain the list
/// @param [out] sz_list
///   pointer to storage for a <tt>uint64_t</tt> which will indicate the number of entries in <tt>list</tt>
/// @return
///   <tt>true</tt> if a list of <tt>uint64_t</tt> was read, otherwise <tt>false</tt>
/// @memberof xchg_message
///
bool xchg_message_read_uint64_list(struct xchg_message *message, uint64_t const *list[], uint64_t *sz_list);

/// Reads a <tt>float32_t</tt> from <tt>message</tt> and advances the underlying buffer position.
///
/// @param [in] message
///   pointer to an <tt>xchg_message</tt> structure
/// @param [out] value
///   pointer to storage for a <tt>float32_t</tt> which will hold the value
/// @return
///   <tt>true</tt> if a <tt>float32_t</tt> was read, otherwise <tt>false</tt>
/// @memberof xchg_message
///
bool xchg_message_read_float32(struct xchg_message *message, float_t *value);

/// Reads a list of <tt>float32_t</tt> from <tt>message</tt> and advances the underlying buffer position.
///
/// @param [in] message
///   pointer to an <tt>xchg_message</tt> structure
/// @param [out] list
///   pointer to storage for a <tt>float32_t[]</tt> which will contain the list
/// @param [out] sz_list
///   pointer to storage for a <tt>uint64_t</tt> which will indicate the number of entries in <tt>list</tt>
/// @return
///   <tt>true</tt> if a list of <tt>float32_t</tt> was read, otherwise <tt>false</tt>
/// @memberof xchg_message
///
bool xchg_message_read_float32_list(struct xchg_message *message, float_t const *list[], uint64_t *sz_list);

/// Reads a <tt>float64_t</tt> from <tt>message</tt> and advances the underlying buffer position.
///
/// @param [in] message
///   pointer to an <tt>xchg_message</tt> structure
/// @param [out] value
///   pointer to storage for a <tt>float64_t</tt> which will hold the value
/// @return
///   <tt>true</tt> if a <tt>float64_t</tt> was read, otherwise <tt>false</tt>
/// @memberof xchg_message
///
bool xchg_message_read_float64(struct xchg_message *message, double_t *value);

/// Reads a list of <tt>float64_t</tt> from <tt>message</tt> and advances the underlying buffer position.
///
/// @param [in] message
///   pointer to an <tt>xchg_message</tt> structure
/// @param [out] list
///   pointer to storage for a <tt>float64_t[]</tt> which will contain the list
/// @param [out] sz_list
///   pointer to storage for a <tt>uint64_t</tt> which will indicate the number of entries in <tt>list</tt>
/// @return
///   <tt>true</tt> if a list of <tt>float64_t</tt> was read, otherwise <tt>false</tt>
/// @memberof xchg_message
///
bool xchg_message_read_float64_list(struct xchg_message *message, double_t const *list[], uint64_t *sz_list);

/// Writes a null <tt>type</tt> to <tt>message</tt> and advances the underlying buffer position.
///
/// @param [in] message
///   pointer to an <tt>xchg_message</tt> structure
/// @param [in] type
///   datatype of the null value
/// @return
///   <tt>true</tt> if a null value was written, otherwise <tt>false</tt>
/// @memberof xchg_message
///
bool xchg_message_write_null(struct xchg_message *message, enum xchg_type type);

/// Writes a null list of <tt>type</tt> to <tt>message</tt> and advances the underlying buffer position.
///
/// @param [in] message
///   pointer to an <tt>xchg_message</tt> structure
/// @param [in] type
///    datatype of entries in the the null list
/// @return
///   <tt>true</tt> if a null list was written, otherwise <tt>false</tt>
/// @memberof xchg_message
///
bool xchg_message_write_null_list(struct xchg_message *message, enum xchg_type type);

/// Writes a <tt>bool</tt> <tt>value</tt> to <tt>message</tt> and advances the underlying buffer position.
///
/// @param [in] message
///   pointer to an <tt>xchg_message</tt> structure
/// @param [in] value
///   <tt>bool</tt> value to write
/// @return
///   <tt>true</tt> if a <tt>bool</tt> was written, otherwise <tt>false</tt>
/// @memberof xchg_message
///
bool xchg_message_write_bool(struct xchg_message *message, bool value);

/// Writes a <tt>bool[]</tt> <tt>list</tt> of size <tt>sz_list</tt> to <tt>message</tt> and advances the
/// underlying buffer position.
///
/// @param [in] message
///   pointer to an <tt>xchg_message</tt> structure
/// @param [in] list
///   list of <tt>bool</tt> values to write
/// @param [in] sz_list
///   number of entries in <tt>list</tt>
/// @return
///   <tt>true</tt> if a <tt>bool[]</tt> was written, otherwise <tt>false</tt>
/// @memberof xchg_message
///
bool xchg_message_write_bool_list(struct xchg_message *message, const bool list[], uint64_t sz_list);

/// Writes an <tt>int8_t</tt> <tt>value</tt> to <tt>message</tt> and advances the underlying buffer position.
///
/// @param [in] message
///   pointer to an <tt>xchg_message</tt> structure
/// @param [in] value
///   <tt>int8_t</tt> value to write
/// @return
///   <tt>true</tt> if an <tt>int8_t</tt> was written, otherwise <tt>false</tt>
/// @memberof xchg_message
///
bool xchg_message_write_int8(struct xchg_message *message, int8_t value);

/// Writes an <tt>int8_t[]</tt> <tt>list</tt> of size <tt>sz_list</tt> to <tt>message</tt> and advances the
/// underlying buffer position.
///
/// @param [in] message
///   pointer to an <tt>xchg_message</tt> structure
/// @param [in] list
///   list of <tt>int8_t</tt> values to write
/// @param [in] sz_list
///   number of entries in <tt>list</tt>
/// @return
///   <tt>true</tt> if an <tt>int8_t[]</tt> was written, otherwise <tt>false</tt>
/// @memberof xchg_message
///
bool xchg_message_write_int8_list(struct xchg_message *message, const int8_t list[], uint64_t sz_list);

/// Writes a <tt>uint8_t</tt> <tt>value</tt> to <tt>message</tt> and advances the underlying buffer position.
///
/// @param [in] message
///   pointer to an <tt>xchg_message</tt> structure
/// @param [in] value
///   <tt>uint8_t</tt> value to write
/// @return
///   <tt>true</tt> if a <tt>uint8_t</tt> was written, otherwise <tt>false</tt>
/// @memberof xchg_message
///
bool xchg_message_write_uint8(struct xchg_message *message, uint8_t value);

/// Writes a <tt>uint8_t[]</tt> <tt>list</tt> of size <tt>sz_list</tt> to <tt>message</tt> and advances the
/// underlying buffer position.
///
/// @param [in] message
///   pointer to an <tt>xchg_message</tt> structure
/// @param [in] list
///   list of <tt>uint8_t</tt> values to write
/// @param [in] sz_list
///   number of entries in <tt>list</tt>
/// @return
///   <tt>true</tt> if a <tt>uint8_t[]</tt> was written, otherwise <tt>false</tt>
/// @memberof xchg_message
///
bool xchg_message_write_uint8_list(struct xchg_message *message, const uint8_t list[], uint64_t sz_list);

/// Writes an <tt>int16_t</tt> <tt>value</tt> to <tt>message</tt> and advances the underlying buffer position.
///
/// @param [in] message
///   pointer to an <tt>xchg_message</tt> structure
/// @param [in] value
///   <tt>int16_t</tt> value to write
/// @return
///   <tt>true</tt> if an <tt>int16_t</tt> was written, otherwise <tt>false</tt>
/// @memberof xchg_message
///
bool xchg_message_write_int16(struct xchg_message *message, int16_t value);

/// Writes an <tt>int16_t[]</tt> <tt>list</tt> of size <tt>sz_list</tt> to <tt>message</tt> and advances the
/// underlying buffer position.
///
/// @param [in] message
///   pointer to an <tt>xchg_message</tt> structure
/// @param [in] list
///   list of <tt>int16_t</tt> values to write
/// @param [in] sz_list
///   number of entries in <tt>list</tt>
/// @return
///   <tt>true</tt> if an <tt>int16_t[]</tt> was written, otherwise <tt>false</tt>
/// @memberof xchg_message
///
bool xchg_message_write_int16_list(struct xchg_message *message, const int16_t list[], uint64_t sz_list);

/// Writes a <tt>uint16_t</tt> <tt>value</tt> to <tt>message</tt> and advances the underlying buffer position.
///
/// @param [in] message
///   pointer to an <tt>xchg_message</tt> structure
/// @param [in] value
///   <tt>uint16_t</tt> value to write
/// @return
///   <tt>true</tt> if a <tt>uint16_t</tt> was written, otherwise <tt>false</tt>
/// @memberof xchg_message
///
bool xchg_message_write_uint16(struct xchg_message *message, uint16_t value);

/// Writes a <tt>uint16_t[]</tt> <tt>list</tt> of size <tt>sz_list</tt> to <tt>message</tt> and advances the
/// underlying buffer position.
///
/// @param [in] message
///   pointer to an <tt>xchg_message</tt> structure
/// @param [in] list
///   list of <tt>uint16_t</tt> values to write
/// @param [in] sz_list
///   number of entries in <tt>list</tt>
/// @return
///   <tt>true</tt> if a <tt>uint16_t[]</tt> was written, otherwise <tt>false</tt>
/// @memberof xchg_message
///
bool xchg_message_write_uint16_list(struct xchg_message *message, const uint16_t list[], uint64_t sz_list);

/// Writes an <tt>int32_t</tt> <tt>value</tt> to <tt>message</tt> and advances the underlying buffer position.
///
/// @param [in] message
///   pointer to an <tt>xchg_message</tt> structure
/// @param [in] value
///   <tt>int32_t</tt> value to write
/// @return
///   <tt>true</tt> if an <tt>int32_t</tt> was written, otherwise <tt>false</tt>
/// @memberof xchg_message
///
bool xchg_message_write_int32(struct xchg_message *message, int32_t value);

/// Writes an <tt>int32_t[]</tt> <tt>list</tt> of size <tt>sz_list</tt> to <tt>message</tt> and advances the
/// underlying buffer position.
///
/// @param [in] message
///   pointer to an <tt>xchg_message</tt> structure
/// @param [in] list
///   list of <tt>int32_t</tt> values to write
/// @param [in] sz_list
///   number of entries in <tt>list</tt>
/// @return
///   <tt>true</tt> if an <tt>int32_t[]</tt> was written, otherwise <tt>false</tt>
/// @memberof xchg_message
///
bool xchg_message_write_int32_list(struct xchg_message *message, const int32_t list[], uint64_t sz_list);

/// Writes a <tt>uint32_t</tt> <tt>value</tt> to <tt>message</tt> and advances the underlying buffer position.
///
/// @param [in] message
///   pointer to an <tt>xchg_message</tt> structure
/// @param [in] value
///   <tt>uint32_t</tt> value to write
/// @return
///   <tt>true</tt> if a <tt>uint32_t</tt> was written, otherwise <tt>false</tt>
/// @memberof xchg_message
///
bool xchg_message_write_uint32(struct xchg_message *message, uint32_t value);

/// Writes a <tt>uint32_t[]</tt> <tt>list</tt> of size <tt>sz_list</tt> to <tt>message</tt> and advances the
/// underlying buffer position.
///
/// @param [in] message
///   pointer to an <tt>xchg_message</tt> structure
/// @param [in] list
///   list of <tt>uint32_t</tt> values to write
/// @param [in] sz_list
///   number of entries in <tt>list</tt>
/// @return
///   <tt>true</tt> if a <tt>uint32_t[]</tt> was written, otherwise <tt>false</tt>
/// @memberof xchg_message
///
bool xchg_message_write_uint32_list(struct xchg_message *message, const uint32_t list[], uint64_t sz_list);

/// Writes an <tt>int64_t</tt> <tt>value</tt> to <tt>message</tt> and advances the underlying buffer position.
///
/// @param [in] message
///   pointer to an <tt>xchg_message</tt> structure
/// @param [in] value
///   <tt>int64_t</tt> value to write
/// @return
///   <tt>true</tt> if an <tt>int64_t</tt> was written, otherwise <tt>false</tt>
/// @memberof xchg_message
///
bool xchg_message_write_int64(struct xchg_message *message, int64_t value);

/// Writes an <tt>int64_t[]</tt> <tt>list</tt> of size <tt>sz_list</tt> to <tt>message</tt> and advances the
/// underlying buffer position.
///
/// @param [in] message
///   pointer to an <tt>xchg_message</tt> structure
/// @param [in] list
///   list of <tt>int64_t</tt> values to write
/// @param [in] sz_list
///   number of entries in <tt>list</tt>
/// @return
///   <tt>true</tt> if an <tt>int64_t[]</tt> was written, otherwise <tt>false</tt>
/// @memberof xchg_message
///
bool xchg_message_write_int64_list(struct xchg_message *message, const int64_t list[], uint64_t sz_list);

/// Writes a <tt>uint64_t</tt> <tt>value</tt> to <tt>message</tt> and advances the underlying buffer position.
///
/// @param [in] message
///   pointer to an <tt>xchg_message</tt> structure
/// @param [in] value
///   <tt>uint64_t</tt> value to write
/// @return
///   <tt>true</tt> if a <tt>uint64_t</tt> was written, otherwise <tt>false</tt>
/// @memberof xchg_message
///
bool xchg_message_write_uint64(struct xchg_message *message, uint64_t value);

/// Writes a <tt>uint64_t[]</tt> <tt>list</tt> of size <tt>sz_list</tt> to <tt>message</tt> and advances the
/// underlying buffer position.
///
/// @param [in] message
///   pointer to an <tt>xchg_message</tt> structure
/// @param [in] list
///   list of <tt>uint64_t</tt> values to write
/// @param [in] sz_list
///   number of entries in <tt>list</tt>
/// @return
///   <tt>true</tt> if a <tt>uint64_t[]</tt> was written, otherwise <tt>false</tt>
/// @memberof xchg_message
///
bool xchg_message_write_uint64_list(struct xchg_message *message, const uint64_t list[], uint64_t sz_list);

/// Writes a <tt>float32_t</tt> <tt>value</tt> to <tt>message</tt> and advances the underlying buffer position.
///
/// @param [in] message
///   pointer to an <tt>xchg_message</tt> structure
/// @param [in] value
///   <tt>float32_t</tt> value to write
/// @return
///   <tt>true</tt> if a <tt>float32_t</tt> was written, otherwise <tt>false</tt>
/// @memberof xchg_message
///
bool xchg_message_write_float32(struct xchg_message *message, float_t value);

/// Writes a <tt>float32_t[]</tt> <tt>list</tt> of size <tt>sz_list</tt> to <tt>message</tt> and advances the
/// underlying buffer position.
///
/// @param [in] message
///   pointer to an <tt>xchg_message</tt> structure
/// @param [in] list
///   list of <tt>float32_t</tt> values to write
/// @param [in] sz_list
///   number of entries in <tt>list</tt>
/// @return
///   <tt>true</tt> if a <tt>float32_t[]</tt> was written, otherwise <tt>false</tt>
/// @memberof xchg_message
///
bool xchg_message_write_float32_list(struct xchg_message *message, const float_t list[], uint64_t sz_list);

/// Writes a <tt>float64_t</tt> <tt>value</tt> to <tt>message</tt> and advances the underlying buffer position.
///
/// @param [in] message
///   pointer to an <tt>xchg_message</tt> structure
/// @param [in] value
///   <tt>float64_t</tt> value to write
/// @return
///   <tt>true</tt> if a <tt>float64_t</tt> was written, otherwise <tt>false</tt>
/// @memberof xchg_message
///
bool xchg_message_write_float64(struct xchg_message *message, double_t value);

/// Writes a <tt>float64_t[]</tt> <tt>list</tt> of size <tt>sz_list</tt> to <tt>message</tt> and advances the
/// underlying buffer position.
///
/// @param [in] message
///   pointer to an <tt>xchg_message</tt> structure
/// @param [in] list
///   list of <tt>float64_t</tt> values to write
/// @param [in] sz_list
///   number of entries in <tt>list</tt>
/// @return
///   <tt>true</tt> if a <tt>float64_t[]</tt> was written, otherwise <tt>false</tt>
/// @memberof xchg_message
///
bool xchg_message_write_float64_list(struct xchg_message *message, const double_t list[], uint64_t sz_list);

/// Represents a lock-free SPSC ring.
///
/// @note
///   This type should be treated as opaque.
/// @private
///
struct xchg_ring
{
    volatile size_t *r;  ///< @private
    size_t cr;  ///< @private
    volatile size_t *w;  ///< @private
    size_t cw;  ///< @private
    char *data;  ///< @private
    size_t sz_data;  ///< @private
    size_t sz_message;  ///< @private
    size_t mask;  ///< @private
};

/// Represents a lock-free communication device backed by shared memory buffers which can be manipulated using the
/// <tt>xchg_channel_*</tt> family of functions.
///
/// @note
///   This type should be treated as opaque.
///
struct xchg_channel
{
    struct xchg_ring ingress;  ///< @private
    struct xchg_ring egress;  ///< @private
    char *error;  ///< @private
};

/// Configures <tt>channel</tt> to use <tt>ingress</tt> of size <tt>sz_ingress</tt> and <tt>egress</tt>
/// of size <tt>sz_egress</tt> as underlying shared memory for receiving and sending messages
/// of size <tt>sz_message</tt>.
///
/// @param [in] channel
///   pointer to an <tt>xchg_channel</tt> structure
/// @param [in] sz_message
///   maximum size, in bytes, of messages that are consumed or produced by this channel
/// @param [in] ingress
///   pointer to the shared memory buffer containing messages to be consumed by this channel
/// @param [in] sz_ingress
///   size, in bytes, of the <tt>ingress</tt> memory buffer
/// @param [in] egress
///   pointer to the shared memory buffer into which messages can be produced by this channel
/// @param [in] sz_egress
///   size, in bytes, of the <tt>egress</tt> memory buffer
/// @return
///   <tt>true</tt> if the provided <tt>xchg_channel</tt> was initialized, or <tt>false</tt> if invalid arguments
///   were provided
/// @note
///   The <tt>sz_message</tt> parameter must be a power-of-two.
/// @note
///   The <tt>sz_ingress</tt> and <tt>sz_egress</tt> parameters must be a power-of-two, plus an additional
///   2*sizeof(void *) bytes for ring accounting overhead.
/// @note
///   The <tt>sz_message</tt> parameter must not be larger than either <tt>sz_ingress</tt> or <tt>sz_egress</tt>.
/// @note
///   While the <tt>ingress</tt> and <tt>egress</tt> parameters are both optional, the caller must provide
///   at least one of them.
/// @memberof xchg_channel
///
bool xchg_channel_init(struct xchg_channel *channel,
                       size_t sz_message, char *ingress, size_t sz_ingress, char *egress, size_t sz_egress);

/// Prepares <tt>message</tt> with writable backing memory from <tt>channel</tt>, allowing the caller to construct
/// a message payload and then send it into the channel via <tt>xchg_channel_send</tt>.
///
/// @param [in] channel
///   pointer to an <tt>xchg_channel</tt> structure
/// @param [in] message
///   pointer to an <tt>xchg_message</tt> to be initialized
/// @return
///   <tt>true</tt> if the provided <tt>xchg_message</tt> was prepared, otherwise <tt>false</tt>
/// @memberof xchg_channel
///
bool xchg_channel_prepare(struct xchg_channel *channel, struct xchg_message *message);

/// Sends <tt>message</tt> (previously initialized via <tt>xchg_channel_prepare</tt>) into <tt>channel</tt>.
///
/// @param [in] channel
///   pointer to an <tt>xchg_channel</tt> structure
/// @param [in] message
///   pointer to an <tt>xchg_message</tt> which was previously initialized by <tt>xchg_channel_prepare</tt>
/// @return
///   <tt>true</tt> if the provided <tt>xchg_message</tt> was sent, otherwise <tt>false</tt>
/// @memberof xchg_channel
///
bool xchg_channel_send(struct xchg_channel *channel, const struct xchg_message *message);

/// Receives the next available message from <tt>channel</tt> into <tt>message</tt>, allowing the caller to
/// read the message payload and then return it to the channel via <tt>xchg_channel_return</tt>.
///
/// @param [in] channel
///   pointer to an <tt>xchg_channel</tt> structure
/// @param [in] message
///   pointer to an <tt>xchg_message</tt> to be initialized
/// @return
///   <tt>true</tt> if a message was received into the provided <tt>xchg_message</tt>, otherwise <tt>false</tt>
/// @memberof xchg_channel
///
bool xchg_channel_receive(struct xchg_channel *channel, struct xchg_message *message);

/// Returns the backing memory of <tt>message</tt> (previously initialized via <tt>xchg_channel_receive</tt>)
/// to <tt>channel</tt>.
///
/// @param [in] channel
///   pointer to an <tt>xchg_channel</tt> structure
/// @param [in] message
///   pointer to an <tt>xchg_message</tt> which was previously initialized by <tt>xchg_channel_receive</tt>
/// @return
///   <tt>true</tt> if the provided <tt>xchg_message</tt> was returned, otherwise <tt>false</tt>
/// @memberof xchg_channel
///
bool xchg_channel_return(struct xchg_channel *channel, const struct xchg_message *message);

/// Provides a static string describing the error that occurred during the last operation on <tt>channel</tt>
///
/// @param [in] channel
///   pointer to an <tt>xchg_channel</tt> structure
/// @return
///   pointer to a static string describing the error that occurred during the last operation on <tt>channel</tt>
/// @memberof xchg_channel
///
const char *xchg_channel_strerror(const struct xchg_channel *channel);

#ifdef __cplusplus
}
#endif
