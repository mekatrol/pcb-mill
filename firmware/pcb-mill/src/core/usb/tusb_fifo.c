#include "tusb_fifo.h"

bool tu_fifo_config(tu_fifo_t* f, void* buffer, uint16_t depth, uint16_t item_size, bool overwritable) {
  // Limit index space to 2*depth - this allows for a fast "modulo" calculation
  // but limits the maximum depth to 2^16/2 = 2^15 and buffer overflows are detectable
  // only if overflow happens once (important for unsupervised DMA applications)
  if (depth > 0x8000) return false;

  f->buffer = (uint8_t*)buffer;
  f->depth = depth;
  f->item_size = (uint16_t)(item_size & 0x7FFF);
  f->overwritable = overwritable;
  f->rd_idx = 0;
  f->wr_idx = 0;

  return true;
}

//--------------------------------------------------------------------+
// Pull & Push
//--------------------------------------------------------------------+

// send n items to fifo WITHOUT updating write pointer
static void _ff_push_n(tu_fifo_t* f, void const* app_buf, uint16_t n, uint16_t wr_ptr) {
  uint16_t const lin_count = f->depth - wr_ptr;
  uint16_t const wrap_count = n - lin_count;

  uint16_t lin_bytes = lin_count * f->item_size;
  uint16_t wrap_bytes = wrap_count * f->item_size;

  // current buffer of fifo
  uint8_t* ff_buf = f->buffer + (wr_ptr * f->item_size);

  if (n <= lin_count) {
    // Linear only
    memcpy(ff_buf, app_buf, n * f->item_size);
  } else {
    // Wrap around

    // Write data to linear part of buffer
    memcpy(ff_buf, app_buf, lin_bytes);

    // Write data wrapped around
    memcpy(f->buffer, ((uint8_t const*)app_buf) + lin_bytes, wrap_bytes);
  }
}

// get one item from fifo WITHOUT updating read pointer
static inline void _ff_pull(tu_fifo_t* f, void* app_buf, uint16_t rel) {
  memcpy(app_buf, f->buffer + (rel * f->item_size), f->item_size);
}

// get n items from fifo WITHOUT updating read pointer
static void _ff_pull_n(tu_fifo_t* f, void* app_buf, uint16_t n, uint16_t rd_ptr) {
  uint16_t const lin_count = f->depth - rd_ptr;
  uint16_t const wrap_count = n - lin_count;  // only used if wrapped

  uint16_t lin_bytes = lin_count * f->item_size;
  uint16_t wrap_bytes = wrap_count * f->item_size;

  // current buffer of fifo
  uint8_t* ff_buf = f->buffer + (rd_ptr * f->item_size);

  if (n <= lin_count) {
    // Linear only
    memcpy(app_buf, ff_buf, n * f->item_size);
  } else {
    // Wrap around

    // Read data from linear part of buffer
    memcpy(app_buf, ff_buf, lin_bytes);

    // Read data wrapped part
    memcpy((uint8_t*)app_buf + lin_bytes, f->buffer, wrap_bytes);
  }
}

//--------------------------------------------------------------------+
// Helper
//--------------------------------------------------------------------+

// return only the index difference and as such can be used to determine an overflow i.e overflowable count
__attribute__((always_inline)) static inline uint16_t _ff_count(uint16_t depth, uint16_t wr_idx, uint16_t rd_idx) {
  // In case we have non-power of two depth we need a further modification
  if (wr_idx >= rd_idx) {
    return (uint16_t)(wr_idx - rd_idx);
  } else {
    return (uint16_t)(2 * depth - (rd_idx - wr_idx));
  }
}

// return remaining slot in fifo
__attribute__((always_inline)) static inline uint16_t _ff_remaining(uint16_t depth, uint16_t wr_idx, uint16_t rd_idx) {
  uint16_t const count = _ff_count(depth, wr_idx, rd_idx);
  return (depth > count) ? (depth - count) : 0;
}

//--------------------------------------------------------------------+
// Index Helper
//--------------------------------------------------------------------+

// Advance an absolute index
// "absolute" index is only in the range of [0..2*depth)
static uint16_t advance_index(uint16_t depth, uint16_t idx, uint16_t offset) {
  // We limit the index space of p such that a correct wrap around happens
  // Check for a wrap around or if we are in unused index space - This has to be checked first!!
  // We are exploiting the wrap around to the correct index
  uint16_t new_idx = (uint16_t)(idx + offset);
  if ((idx > new_idx) || (new_idx >= 2 * depth)) {
    uint16_t const non_used_index_space = (uint16_t)(UINT16_MAX - (2 * depth - 1));
    new_idx = (uint16_t)(new_idx + non_used_index_space);
  }

  return new_idx;
}

#if 0  // not used but
// Backward an absolute index
static uint16_t backward_index(uint16_t depth, uint16_t idx, uint16_t offset)
{
  // We limit the index space of p such that a correct wrap around happens
  // Check for a wrap around or if we are in unused index space - This has to be checked first!!
  // We are exploiting the wrap around to the correct index
  uint16_t new_idx = (uint16_t) (idx - offset);
  if ( (idx < new_idx) || (new_idx >= 2*depth) )
  {
    uint16_t const non_used_index_space = (uint16_t) (UINT16_MAX - (2*depth-1));
    new_idx = (uint16_t) (new_idx - non_used_index_space);
  }

  return new_idx;
}
#endif

// index to pointer, simply an modulo with minus.
__attribute__((always_inline)) static inline uint16_t idx2ptr(uint16_t depth, uint16_t idx) {
  // Only run at most 3 times since index is limit in the range of [0..2*depth)
  while (idx >= depth) idx -= depth;
  return idx;
}

// Works on local copies of w
// When an overwritable fifo is overflowed, rd_idx will be re-index so that it forms
// an full fifo i.e _ff_count() = depth
__attribute__((always_inline)) static inline uint16_t _ff_correct_read_index(tu_fifo_t* f, uint16_t wr_idx) {
  uint16_t rd_idx;
  if (wr_idx >= f->depth) {
    rd_idx = wr_idx - f->depth;
  } else {
    rd_idx = wr_idx + f->depth;
  }

  f->rd_idx = rd_idx;

  return rd_idx;
}

// Works on local copies of w and r
static bool _tu_fifo_peek(tu_fifo_t* f, void* p_buffer, uint16_t wr_idx, uint16_t rd_idx) {
  uint16_t cnt = _ff_count(f->depth, wr_idx, rd_idx);

  // nothing to peek
  if (cnt == 0) return false;

  // Check overflow and correct if required
  if (cnt > f->depth) {
    rd_idx = _ff_correct_read_index(f, wr_idx);
    cnt = f->depth;
  }

  uint16_t rd_ptr = idx2ptr(f->depth, rd_idx);

  // Peek data
  _ff_pull(f, p_buffer, rd_ptr);

  return true;
}

// Works on local copies of w and r
static uint16_t _tu_fifo_peek_n(tu_fifo_t* f, void* p_buffer, uint16_t n, uint16_t wr_idx, uint16_t rd_idx) {
  uint16_t cnt = _ff_count(f->depth, wr_idx, rd_idx);

  // nothing to peek
  if (cnt == 0) return 0;

  // Check overflow and correct if required
  if (cnt > f->depth) {
    rd_idx = _ff_correct_read_index(f, wr_idx);
    cnt = f->depth;
  }

  // Check if we can read something at and after offset - if too less is available we read what remains
  if (cnt < n) n = cnt;

  uint16_t rd_ptr = idx2ptr(f->depth, rd_idx);

  // Peek data
  _ff_pull_n(f, p_buffer, n, rd_ptr);

  return n;
}

static uint16_t _tu_fifo_write_n(tu_fifo_t* f, const void* data, uint16_t n) {
  if (n == 0) return 0;

  uint16_t wr_idx = f->wr_idx;
  uint16_t rd_idx = f->rd_idx;

  uint8_t const* buf8 = (uint8_t const*)data;

  if (!f->overwritable) {
    // limit up to full
    uint16_t const remain = _ff_remaining(f->depth, wr_idx, rd_idx);
    n = min_u16(n, remain);
  } else {
    // In over-writable mode, fifo_write() is allowed even when fifo is full. In such case,
    // oldest data in fifo i.e at read pointer data will be overwritten
    // Note: we can modify read buffer contents but we must not modify the read index itself within a write function!
    // Since it would end up in a race condition with read functions!
    if (n >= f->depth) {
      // Only copy last part
      buf8 += (n - f->depth) * f->item_size;
      n = f->depth;

      // We start writing at the read pointer's position since we fill the whole buffer
      wr_idx = rd_idx;
    } else {
      uint16_t const overflowable_count = _ff_count(f->depth, wr_idx, rd_idx);
      if (overflowable_count + n >= 2 * f->depth) {
        // Double overflowed
        // Index is bigger than the allowed range [0,2*depth)
        // re-position write index to have a full fifo after pushed
        wr_idx = advance_index(f->depth, rd_idx, f->depth - n);

        // TODO we should also shift out n bytes from read index since we avoid changing rd index !!
        // However memmove() is expensive due to actual copying + wrapping consideration.
        // Also race condition could happen anyway if read() is invoke while moving result in corrupted memory
        // currently deliberately not implemented --> result in incorrect data read back
      } else {
        // normal + single overflowed:
        // Index is in the range of [0,2*depth) and thus detect and recoverable. Recovering is handled in read()
        // Therefore we just increase write index
        // we will correct (re-position) read index later on in fifo_read() function
      }
    }
  }

  if (n) {
    uint16_t wr_ptr = idx2ptr(f->depth, wr_idx);

    // Write data
    _ff_push_n(f, buf8, n, wr_ptr);

    // Advance index
    f->wr_idx = advance_index(f->depth, wr_idx, n);
  }

  return n;
}

static uint16_t _tu_fifo_read_n(tu_fifo_t* f, void* buffer, uint16_t n) {
  // Peek the data
  // f->rd_idx might get modified in case of an overflow so we can not use a local variable
  n = _tu_fifo_peek_n(f, buffer, n, f->wr_idx, f->rd_idx);

  // Advance read pointer
  f->rd_idx = advance_index(f->depth, f->rd_idx, n);

  return n;
}

//--------------------------------------------------------------------+
// Application API
//--------------------------------------------------------------------+

/******************************************************************************/
/*!
    @brief Get number of items in FIFO.

    As this function only reads the read and write pointers once, this function is
    reentrant. In case an
    overflow occurred, this function return f.depth at maximum. Overflows are
    checked and corrected for in the read functions!

    @param[in]  f
                Pointer to the FIFO buffer to manipulate

    @returns Number of items in FIFO
 */
/******************************************************************************/
uint16_t tu_fifo_count(tu_fifo_t* f) {
  return min_u16(_ff_count(f->depth, f->wr_idx, f->rd_idx), f->depth);
}

/******************************************************************************/
/*!
    @brief Check if FIFO is empty.

    As this function only reads the read and write pointers once, this function is
    reentrant.

    @param[in]  f
                Pointer to the FIFO buffer to manipulate

    @returns Number of items in FIFO
 */
/******************************************************************************/
bool tu_fifo_empty(tu_fifo_t* f) {
  return f->wr_idx == f->rd_idx;
}

/******************************************************************************/
/*!
    @brief Check if FIFO is full.

    As this function only reads the read and write pointers once, this function is
    reentrant.

    @param[in]  f
                Pointer to the FIFO buffer to manipulate

    @returns Number of items in FIFO
 */
/******************************************************************************/
bool tu_fifo_full(tu_fifo_t* f) {
  return _ff_count(f->depth, f->wr_idx, f->rd_idx) >= f->depth;
}

/******************************************************************************/
/*!
    @brief Get remaining space in FIFO.

    As this function only reads the read and write pointers once, this function is
    reentrant.

    @param[in]  f
                Pointer to the FIFO buffer to manipulate

    @returns Number of items in FIFO
 */
/******************************************************************************/
uint16_t tu_fifo_remaining(tu_fifo_t* f) {
  return _ff_remaining(f->depth, f->wr_idx, f->rd_idx);
}

/******************************************************************************/
/*!
    @brief Check if overflow happened.

     BE AWARE - THIS FUNCTION MIGHT NOT GIVE A CORRECT ANSWERE IN CASE WRITE POINTER "OVERFLOWS"
     Only one overflow is allowed for this function to work e.g. if depth = 100, you must not
     write more than 2*depth-1 items in one rush without updating write pointer. Otherwise
     write pointer wraps and your pointer states are messed up. This can only happen if you
     use DMAs, write functions do not allow such an error. Avoid such nasty things!

     All reading functions (read, peek) check for overflows and correct read pointer on their own such
     that latest items are read.
     If required (e.g. for DMA use) you can also correct the read pointer by
     tu_fifo_correct_read_pointer().

    @param[in]  f
                Pointer to the FIFO buffer to manipulate

    @returns True if overflow happened
 */
/******************************************************************************/
bool tu_fifo_overflowed(tu_fifo_t* f) {
  return _ff_count(f->depth, f->wr_idx, f->rd_idx) > f->depth;
}

// Only use in case tu_fifo_overflow() returned true!
void tu_fifo_correct_read_pointer(tu_fifo_t* f) {
  _ff_correct_read_index(f, f->wr_idx);
}

/******************************************************************************/
/*!
    @brief Read one element out of the buffer.

    This function will return the element located at the array index of the
    read pointer, and then increment the read pointer index.
    This function checks for an overflow and corrects read pointer if required.

    @param[in]  f
                Pointer to the FIFO buffer to manipulate
    @param[in]  buffer
                Pointer to the place holder for data read from the buffer

    @returns TRUE if the queue is not empty
 */
/******************************************************************************/
bool tu_fifo_read(tu_fifo_t* f, void* buffer) {
  // Peek the data
  // f->rd_idx might get modified in case of an overflow so we can not use a local variable
  bool ret = _tu_fifo_peek(f, buffer, f->wr_idx, f->rd_idx);

  // Advance pointer
  f->rd_idx = advance_index(f->depth, f->rd_idx, ret);
  return ret;
}

/******************************************************************************/
/*!
    @brief This function will read n elements from the array index specified by
    the read pointer and increment the read index.
    This function checks for an overflow and corrects read pointer if required.

    @param[in]  f
                Pointer to the FIFO buffer to manipulate
    @param[in]  buffer
                The pointer to data location
    @param[in]  n
                Number of element that buffer can afford

    @returns number of items read from the FIFO
 */
/******************************************************************************/
uint16_t tu_fifo_read_n(tu_fifo_t* f, void* buffer, uint16_t n) {
  return _tu_fifo_read_n(f, buffer, n);
}

/******************************************************************************/
/*!
    @brief Write one element into the buffer.

    This function will write one element into the array index specified by
    the write pointer and increment the write index.

    @param[in]  f
                Pointer to the FIFO buffer to manipulate
    @param[in]  data
                The byte to add to the FIFO

    @returns TRUE if the data was written to the FIFO (overwrittable
             FIFO will always return TRUE)
 */
/******************************************************************************/
bool tu_fifo_write(tu_fifo_t* f, const void* data) {
  bool ret;
  uint16_t const wr_idx = f->wr_idx;

  if (tu_fifo_full(f) && !f->overwritable) {
    ret = false;
  } else {
    uint16_t wr_ptr = idx2ptr(f->depth, wr_idx);

    // Write data
    memcpy(f->buffer + (wr_ptr * f->item_size), data, f->item_size);

    // Advance pointer
    f->wr_idx = advance_index(f->depth, wr_idx, 1);

    ret = true;
  }

  return ret;
}

/******************************************************************************/
/*!
    @brief This function will write n elements into the array index specified by
    the write pointer and increment the write index.

    @param[in]  f
                Pointer to the FIFO buffer to manipulate
    @param[in]  data
                The pointer to data to add to the FIFO
    @param[in]  count
                Number of element
    @return Number of written elements
 */
/******************************************************************************/
uint16_t tu_fifo_write_n(tu_fifo_t* f, const void* data, uint16_t n) {
  return _tu_fifo_write_n(f, data, n);
}

/******************************************************************************/
/*!
    @brief Clear the fifo read and write pointers

    @param[in]  f
                Pointer to the FIFO buffer to manipulate
 */
/******************************************************************************/
void tu_fifo_clear(tu_fifo_t* f) {
  f->rd_idx = 0;
  f->wr_idx = 0;
}

/******************************************************************************/
/*!
    @brief Advance write pointer - intended to be used in combination with DMA.
    It is possible to fill the FIFO by use of a DMA in circular mode. Within
    DMA ISRs you may update the write pointer to be able to read from the FIFO.
    As long as the DMA is the only process writing into the FIFO this is safe
    to use.

    USE WITH CARE - WE DO NOT CONDUCT SAFETY CHECKS HERE!

    @param[in]  f
                Pointer to the FIFO buffer to manipulate
    @param[in]  n
                Number of items the write pointer moves forward
 */
/******************************************************************************/
void tu_fifo_advance_write_pointer(tu_fifo_t* f, uint16_t n) {
  f->wr_idx = advance_index(f->depth, f->wr_idx, n);
}

/******************************************************************************/
/*!
    @brief Advance read pointer - intended to be used in combination with DMA.
    It is possible to read from the FIFO by use of a DMA in linear mode. Within
    DMA ISRs you may update the read pointer to be able to again write into the
    FIFO. As long as the DMA is the only process reading from the FIFO this is
    safe to use.

    USE WITH CARE - WE DO NOT CONDUCT SAFETY CHECKS HERE!

    @param[in]  f
                Pointer to the FIFO buffer to manipulate
    @param[in]  n
                Number of items the read pointer moves forward
 */
/******************************************************************************/
void tu_fifo_advance_read_pointer(tu_fifo_t* f, uint16_t n) {
  f->rd_idx = advance_index(f->depth, f->rd_idx, n);
}

/******************************************************************************/
/*!
   @brief Get read info

   Returns the length and pointer from which bytes can be read in a linear manner.
   This is of major interest for DMA transmissions. If returned length is zero the
   corresponding pointer is invalid.
   The read pointer does NOT get advanced, use tu_fifo_advance_read_pointer() to
   do so!
   @param[in]       f
                    Pointer to FIFO
   @param[out]      *info
                    Pointer to struct which holds the desired infos
 */
/******************************************************************************/
void tu_fifo_get_read_info(tu_fifo_t* f, tu_fifo_buffer_info_t* info) {
  // Operate on temporary values in case they change in between
  uint16_t wr_idx = f->wr_idx;
  uint16_t rd_idx = f->rd_idx;

  uint16_t cnt = _ff_count(f->depth, wr_idx, rd_idx);

  // Check overflow and correct if required - may happen in case a DMA wrote too fast
  if (cnt > f->depth) {
    rd_idx = _ff_correct_read_index(f, wr_idx);

    cnt = f->depth;
  }

  // Check if fifo is empty
  if (cnt == 0) {
    info->len_lin = 0;
    info->len_wrap = 0;
    info->ptr_lin = NULL;
    info->ptr_wrap = NULL;
    return;
  }

  // Get relative pointers
  uint16_t wr_ptr = idx2ptr(f->depth, wr_idx);
  uint16_t rd_ptr = idx2ptr(f->depth, rd_idx);

  // Copy pointer to buffer to start reading from
  info->ptr_lin = &f->buffer[rd_ptr];

  // Check if there is a wrap around necessary
  if (wr_ptr > rd_ptr) {
    // Non wrapping case
    info->len_lin = cnt;

    info->len_wrap = 0;
    info->ptr_wrap = NULL;
  } else {
    info->len_lin = f->depth - rd_ptr;  // Also the case if FIFO was full

    info->len_wrap = cnt - info->len_lin;
    info->ptr_wrap = f->buffer;
  }
}

/******************************************************************************/
/*!
   @brief Get linear write info

   Returns the length and pointer to which bytes can be written into FIFO in a linear manner.
   This is of major interest for DMA transmissions not using circular mode. If a returned length is zero the
   corresponding pointer is invalid. The returned lengths summed up are the currently free space in the FIFO.
   The write pointer does NOT get advanced, use tu_fifo_advance_write_pointer() to do so!
   TAKE CARE TO NOT OVERFLOW THE BUFFER MORE THAN TWO TIMES THE FIFO DEPTH - IT CAN NOT RECOVERE OTHERWISE!
   @param[in]       f
                    Pointer to FIFO
   @param[out]      *info
                    Pointer to struct which holds the desired infos
 */
/******************************************************************************/
void tu_fifo_get_write_info(tu_fifo_t* f, tu_fifo_buffer_info_t* info) {
  uint16_t wr_idx = f->wr_idx;
  uint16_t rd_idx = f->rd_idx;
  uint16_t remain = _ff_remaining(f->depth, wr_idx, rd_idx);

  if (remain == 0) {
    info->len_lin = 0;
    info->len_wrap = 0;
    info->ptr_lin = NULL;
    info->ptr_wrap = NULL;
    return;
  }

  // Get relative pointers
  uint16_t wr_ptr = idx2ptr(f->depth, wr_idx);
  uint16_t rd_ptr = idx2ptr(f->depth, rd_idx);

  // Copy pointer to buffer to start writing to
  info->ptr_lin = &f->buffer[wr_ptr];

  if (wr_ptr < rd_ptr) {
    // Non wrapping case
    info->len_lin = rd_ptr - wr_ptr;
    info->len_wrap = 0;
    info->ptr_wrap = NULL;
  } else {
    info->len_lin = f->depth - wr_ptr;
    info->len_wrap = remain - info->len_lin;  // Remaining length - n already was limited to remain or FIFO depth
    info->ptr_wrap = f->buffer;               // Always start of buffer
  }
}
