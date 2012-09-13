// [AsmJit]
// Complete JIT Assembler for C++ Language.
//
// [License]
// Zlib - See COPYING file in this package.

// [Guard]
#ifndef _ASMJIT_CORE_PODVECTOR_H
#define _ASMJIT_CORE_PODVECTOR_H

// [Dependencies - AsmJit]
#include "../core/assert.h"
#include "../core/defs.h"

namespace AsmJit {

//! @addtogroup AsmJit_Core
//! @{

// ============================================================================
// [AsmJit::PodVector<T>]
// ============================================================================

//! @brief Template used to store and manage array of POD data.
//!
//! This template has these adventages over other vector<> templates:
//! - Non-copyable (designed to be non-copyable, we want it)
//! - No copy-on-write (some implementations of stl can use it)
//! - Optimized for working only with POD types
//! - Uses ASMJIT_... memory management macros
template <typename T>
struct PodVector
{
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! @brief Create new instance of PodVector template. Data will not
  //! be allocated (will be NULL).
  inline PodVector() ASMJIT_NOTHROW :
    _data(NULL),
    _length(0),
    _capacity(0)
  {
  }
  
  //! @brief Destroy PodVector and free all data.
  inline ~PodVector() ASMJIT_NOTHROW
  {
    if (_data != NULL)
      ASMJIT_FREE(_data);
  }

  // --------------------------------------------------------------------------
  // [Data]
  // --------------------------------------------------------------------------

  //! @brief Get data.
  inline T* getData() ASMJIT_NOTHROW { return _data; }
  //! @overload
  inline const T* getData() const ASMJIT_NOTHROW { return _data; }

  //! @brief Get length.
  inline size_t getLength() const ASMJIT_NOTHROW { return _length; }
  //! @brief Get capacity.
  inline size_t getCapacity() const ASMJIT_NOTHROW { return _capacity; }

  // --------------------------------------------------------------------------
  // [Manipulation]
  // --------------------------------------------------------------------------

  //! @brief Clear vector data, but not free internal buffer.
  void clear() ASMJIT_NOTHROW
  {
    _length = 0;
  }

  //! @brief Clear vector data and free internal buffer.
  void reset() ASMJIT_NOTHROW
  {
    if (_data != NULL) 
    {
      ASMJIT_FREE(_data);
      _data = 0;
      _length = 0;
      _capacity = 0;
    }
  }

  //! @brief Prepend @a item to vector.
  bool prepend(const T& item) ASMJIT_NOTHROW
  {
    if (_length == _capacity && !_grow()) return false;

    memmove(_data + 1, _data, sizeof(T) * _length);
    memcpy(_data, &item, sizeof(T));

    _length++;
    return true;
  }

  //! @brief Insert an @a item at the @a index.
  bool insert(size_t index, const T& item) ASMJIT_NOTHROW
  {
    ASMJIT_ASSERT(index <= _length);
    if (_length == _capacity && !_grow()) return false;

    T* dst = _data + index;
    memmove(dst + 1, dst, _length - index);
    memcpy(dst, &item, sizeof(T));

    _length++;
    return true;
  }

  //! @brief Append @a item to vector.
  bool append(const T& item) ASMJIT_NOTHROW
  {
    if (_length == _capacity && !_grow()) return false;

    memcpy(_data + _length, &item, sizeof(T));

    _length++;
    return true;
  }

  //! @brief Get index of @a val or kInvalidSize if not found.
  size_t indexOf(const T& val) const ASMJIT_NOTHROW
  {
    size_t i = 0, len = _length;
    for (i = 0; i < len; i++) { if (_data[i] == val) return i; }
    return kInvalidSize;
  }

  //! @brief Remove element at index @a i.
  void removeAt(size_t i) ASMJIT_NOTHROW
  {
    ASMJIT_ASSERT(i < _length);

    T* dst = _data + i;
    _length--;
    memmove(dst, dst + 1, _length - i);
  }

  //! @brief Swap this pod-vector with @a other.
  void swap(PodVector<T>& other) ASMJIT_NOTHROW
  {
    T* _tmp_data = _data;
    size_t _tmp_length = _length;
    size_t _tmp_capacity = _capacity;

    _data = other._data;
    _length = other._length;
    _capacity = other._capacity;

    other._data = _tmp_data;
    other._length = _tmp_length;
    other._capacity = _tmp_capacity;
  }

  //! @brief Get item at position @a i.
  inline T& operator[](size_t i) ASMJIT_NOTHROW
  {
    ASMJIT_ASSERT(i < _length);
    return _data[i];
  }
  //! @brief Get item at position @a i.
  inline const T& operator[](size_t i) const ASMJIT_NOTHROW
  {
    ASMJIT_ASSERT(i < _length);
    return _data[i];
  }

  //! @brief Append the item and return address so it can be initialized.
  T* newItem() ASMJIT_NOTHROW
  {
    if (_length == _capacity && !_grow()) return NULL;
    return _data + (_length++);
  }

  // --------------------------------------------------------------------------
  // [Private]
  // --------------------------------------------------------------------------

  //! @brief Called to grow internal array.
  bool _grow() ASMJIT_NOTHROW
  {
    return _realloc(_capacity < 16 ? 16 : _capacity * 2);
  }

  //! @brief Realloc internal array to fit @a to items.
  bool _realloc(size_t to) ASMJIT_NOTHROW
  {
    ASMJIT_ASSERT(to >= _length);

    T* p = reinterpret_cast<T*>(_data ? ASMJIT_REALLOC(_data, to * sizeof(T)) : ASMJIT_MALLOC(to * sizeof(T)));

    if (p == NULL)
      return false;

    _data = p;
    _capacity = to;
    return true;
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! @brief Items data.
  T* _data;
  //! @brief Length of buffer (count of items in array).
  size_t _length;
  //! @brief Capacity of buffer (maximum items that can fit to current array).
  size_t _capacity;

  ASMJIT_NO_COPY(PodVector<T>)
};

//! @}

} // AsmJit namespace

#endif // _ASMJIT_CORE_PODVECTOR_H
