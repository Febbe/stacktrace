// Copyright Fabian Keßler 2022 - 2023.

// Modified version of stacktrace from GCC
// https://raw.githubusercontent.com/gcc-mirror/gcc/7b206ae7f17455b69349767ec48b074db260a2a7/libstdc%2B%2B-v3/include/std/stacktrace
// :

// <stacktrace> -*- C++ -*-

// Copyright The GNU Toolchain Authors.
//
// This file is part of the GNU ISO C++ Library.  This library is free
// software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the
// Free Software Foundation; either version 3.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// Under Section 7 of GPL version 3, you are granted additional
// permissions described in the GCC Runtime Library Exception, version
// 3.1, as published by the Free Software Foundation.

// You should have received a copy of the GNU General Public License and
// a copy of the GCC Runtime Library Exception along with this program;
// see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
// <http://www.gnu.org/licenses/>.

#pragma once
#ifndef _FBBE_GNU_STACKTRACE
#define _FBBE_GNU_STACKTRACE 1

#if not defined(_FBBE_GNU_STACKTRACE)
#if __has_include(<stacktrace>)
#include <stacktrace>
#if defined(__cpp_lib_stacktrace) && __cpp_lib_stacktrace >= 201907L
#define _FBBE_GNU_STACKTRACE 0
#else
#define _FBBE_GNU_STACKTRACE 1
#endif
#endif
#else
#define _FBBE_GNU_STACKTRACE 1
#endif

#if _FBBE_GNU_STACKTRACE == 0

namespace fbbe {
using stacktrace_entry = ::std::stacktrace_entry;
template <class _Alloc>
using basic_stacktrace = ::std::basic_stacktrace<_Alloc>;
using stacktrace = ::std::stacktrace;
namespace pmr {
using stacktrace = std::pmr::stacktrace;
}
} // namespace fbbe

#endif

#pragma GCC system_header

#include <compare>
#include <limits>
#include <memory>
#include <new>
#include <sstream>
#include <string>
#include <utility>


#if __has_include(<backtrace.h>)
#include <backtrace.h>
#else

struct backtrace_state;
struct backtrace_simple_data;
extern "C" {
backtrace_state *backtrace_create_state(const char *, int,
                                        void (*)(void *, const char *, int),
                                        void *);
int backtrace_simple(backtrace_state *, int, int (*)(void *, __UINTPTR_TYPE__),
                     void (*)(void *, const char *, int), void *);
int backtrace_pcinfo(backtrace_state *, __UINTPTR_TYPE__,
                     int (*)(void *, __UINTPTR_TYPE__, const char *, int,
                             const char *),
                     void (*)(void *, const char *, int), void *);
int backtrace_syminfo(backtrace_state *, __UINTPTR_TYPE__ addr,
                      void (*)(void *, __UINTPTR_TYPE__, const char *,
                               __UINTPTR_TYPE__, __UINTPTR_TYPE__),
                      void (*)(void *, const char *, int), void *);
}

#endif

#if defined(_LIBCPP_VERSION) && __has_include(<__assert>)
#    include <__assert>
#    define _FBBE_ASSERT(pred) _LIBCPP_ASSERT(pred, "")
#elif defined(__GLIBCXX__)
#    define _FBBE_ASSERT(pred) __glibcxx_assert(pred)
#else
#    include <cassert>
#    define _FBBE_ASSERT(pred) (assert((pred)))
#endif

#if defined(__GLIBCXX__) || defined(_WIN32) || defined(CYGWIN)
  #define _FBBE_TRY __try 
  #define _FBBE_CATCH(x) __catch(x)
#else
  #define _FBBE_TRY try
  #define _FBBE_CATCH(x) catch(x)
#endif



namespace __cxxabiv1 {
extern "C" char *__cxa_demangle(const char *__mangled_name,
                                char *__output_buffer, size_t *__length,
                                int *__status);
}

namespace fbbe {

// [stacktrace.entry], class stacktrace_entry
class stacktrace_entry {
  using uint_least32_t = __UINT_LEAST32_TYPE__;
  using uintptr_t = __UINTPTR_TYPE__;

public:
  using native_handle_type = uintptr_t;

  // [stacktrace.entry.ctor], constructors

  constexpr stacktrace_entry() noexcept = default;

  constexpr stacktrace_entry(const stacktrace_entry &__other) noexcept =
      default;

  constexpr stacktrace_entry &
  operator=(const stacktrace_entry &__other) noexcept = default;

  ~stacktrace_entry() = default;

  // [stacktrace.entry.obs], observers

  constexpr native_handle_type native_handle() const noexcept { return _M_pc; }

  constexpr explicit operator bool() const noexcept { return _M_pc != -1; }

  // [stacktrace.entry.query], query
  std::string description() const {
    std::string __s;
    _M_get_info(&__s, nullptr, nullptr);
    return __s;
  }

  std::string source_file() const {
    std::string __s;
    _M_get_info(nullptr, &__s, nullptr);
    return __s;
  }

  uint_least32_t source_line() const {
    int __line = 0;
    _M_get_info(nullptr, nullptr, &__line);
    return __line;
  }

  // [stacktrace.entry.cmp], comparison
#if _HAS_CXX20

  [[nodiscard]] friend constexpr bool
  operator==(const stacktrace_entry &,
             const stacktrace_entry &) noexcept = default;
  [[nodiscard]] friend constexpr std::strong_ordering
  operator<=>(const stacktrace_entry &,
              const stacktrace_entry &) noexcept = default;

#else

  [[nodiscard]] friend constexpr bool
  operator==(const stacktrace_entry &lhs,
             const stacktrace_entry &rhs) noexcept {
    return lhs._M_pc == rhs._M_pc;
  }

  [[nodiscard]] friend constexpr bool
  operator!=(const stacktrace_entry &lhs,
             const stacktrace_entry &rhs) noexcept {
    return lhs._M_pc != rhs._M_pc;
  }

  [[nodiscard]] friend constexpr bool
  operator<(const stacktrace_entry &lhs, const stacktrace_entry &rhs) noexcept {
    return lhs._M_pc < rhs._M_pc;
  }

  [[nodiscard]] friend constexpr bool
  operator<=(const stacktrace_entry &lhs,
             const stacktrace_entry &rhs) noexcept {
    return lhs._M_pc <= rhs._M_pc;
  }

  [[nodiscard]] friend constexpr bool
  operator>(const stacktrace_entry &lhs, const stacktrace_entry &rhs) noexcept {
    return lhs._M_pc > rhs._M_pc;
  }

  [[nodiscard]] friend constexpr bool
  operator>=(const stacktrace_entry &lhs,
             const stacktrace_entry &rhs) noexcept {
    return lhs._M_pc >= rhs._M_pc;
  }

#endif

private:
  native_handle_type _M_pc = -1;

  template <typename _Allocator> friend class basic_stacktrace;

  static void _S_err_handler(void *, const char *, int) {}

  static backtrace_state *_S_init() {
    static backtrace_state *__state =
        backtrace_create_state(nullptr, 1, _S_err_handler, nullptr);
    return __state;
  }

  friend std::ostream &operator<<(std::ostream &, const stacktrace_entry &);

  bool _M_get_info(std::string *__desc, std::string *__file,
                   int *__line) const {
    if (!*this)
      return false;

    struct _Data {
      std::string *_M_desc;
      std::string *_M_file;
      int *_M_line;
    } __data = {__desc, __file, __line};

    auto __cb = [](void *__data, uintptr_t, const char *__filename,
                   int __lineno, const char *__function) -> int {
      auto &__d = *static_cast<_Data *>(__data);
      if (__function && __d._M_desc)
        *__d._M_desc = _S_demangle(__function);
      if (__filename && __d._M_file)
        *__d._M_file = __filename;
      if (__d._M_line)
        *__d._M_line = __lineno;
      return __function != nullptr;
    };
    const auto __state = _S_init();
    if (::backtrace_pcinfo(__state, _M_pc, +__cb, _S_err_handler, &__data))
      return true;
    if (__desc && __desc->empty()) {
      auto __cb2 = [](void *__data, uintptr_t, const char *__symname, uintptr_t,
                      uintptr_t) {
        if (__symname)
          *static_cast<_Data *>(__data)->_M_desc = _S_demangle(__symname);
      };
      if (::backtrace_syminfo(__state, _M_pc, +__cb2, _S_err_handler, &__data))
        return true;
    }
    return false;
  }

#if __has_builtin(__builtin_free)
#define _FBBE_GNU_FREE __builtin_free
#else
#define _FBBE_GNU_FREE std::free
#endif

  static std::string _S_demangle(const char *__name) {
    std::string __s;
    int __status;
    char *__str =
        __cxxabiv1::__cxa_demangle(__name, nullptr, nullptr, &__status);
    if (__status == 0)
      __s = __str;
    else
      __s = __name;
    _FBBE_GNU_FREE(__str);
    return __s;
  }
};

// [stacktrace.basic], class template basic_stacktrace
template <typename _Allocator> class basic_stacktrace {
  using _AllocTraits = std::allocator_traits<_Allocator>;
  using uintptr_t = __UINTPTR_TYPE__;

public:
  using value_type = stacktrace_entry;
  using const_reference = const value_type &;
  using reference = value_type &;
  using const_iterator = value_type const *;
  using iterator = const_iterator;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  using difference_type = ptrdiff_t;
  using size_type = unsigned short;
  using allocator_type = _Allocator;

  // [stacktrace.basic.ctor], creation and assignment

  [[__gnu__::__noinline__]] static basic_stacktrace
  current(const allocator_type &__alloc = allocator_type()) noexcept {
    basic_stacktrace __ret(__alloc);
    if (auto __cb = __ret._M_prepare()) [[likely]] {
      auto __state = stacktrace_entry::_S_init();
      if (backtrace_simple(__state, 1, __cb, stacktrace_entry::_S_err_handler,
                           std::addressof(__ret)))
        __ret._M_clear();
    }
    return __ret;
  }

  [[__gnu__::__noinline__]] static basic_stacktrace
  current(size_type __skip,
          const allocator_type &__alloc = allocator_type()) noexcept {
    basic_stacktrace __ret(__alloc);
    if (__skip >= __INT_MAX__) [[unlikely]]
      return __ret;
    if (auto __cb = __ret._M_prepare()) [[likely]] {
      auto __state = stacktrace_entry::_S_init();
      if (backtrace_simple(__state, __skip + 1, __cb,
                           stacktrace_entry::_S_err_handler,
                           std::addressof(__ret)))
        __ret._M_clear();
    }

    return __ret;
  }

  [[__gnu__::__noinline__]] static basic_stacktrace
  current(size_type __skip, size_type __max_depth,
          const allocator_type &__alloc = allocator_type()) noexcept {
    _FBBE_ASSERT(__skip <= (size_type(-1) - __max_depth));

    basic_stacktrace __ret(__alloc);
    if (__max_depth == 0) [[unlikely]]
      return __ret;
    if (__skip >= __INT_MAX__) [[unlikely]]
      return __ret;
    if (auto __cb = __ret._M_prepare(__max_depth)) [[likely]] {
      auto __state = stacktrace_entry::_S_init();
      int __err = backtrace_simple(__state, __skip + 1, __cb,
                                   stacktrace_entry::_S_err_handler,
                                   std::addressof(__ret));
      if (__err < 0)
        __ret._M_clear();
      else if (__ret.size() > __max_depth) {
        __ret._M_impl._M_resize(__max_depth, __ret._M_alloc);

        if (__ret._M_impl._M_capacity / 2 >= __max_depth) {
          // shrink to fit
          _Impl __tmp = __ret._M_impl._M_clone(__ret._M_alloc);
          if (__tmp._M_capacity) {
            __ret._M_clear();
            __ret._M_impl = __tmp;
          }
        }
      }
    }
    return __ret;
  }

  basic_stacktrace() noexcept(
      std::is_nothrow_default_constructible_v<allocator_type>) {}

  explicit basic_stacktrace(const allocator_type &__alloc) noexcept
      : _M_alloc(__alloc) {}

  basic_stacktrace(const basic_stacktrace &__other) noexcept
      : basic_stacktrace(__other,
                         _AllocTraits::select_on_container_copy_construction(
                             __other._M_alloc)) {}

  basic_stacktrace(basic_stacktrace &&__other) noexcept
      : _M_alloc(std::move(__other._M_alloc)),
        _M_impl(std::exchange(__other._M_impl, {})) {}

  basic_stacktrace(const basic_stacktrace &__other,
                   const allocator_type &__alloc) noexcept
      : _M_alloc(__alloc) {
    if (const auto __s = __other._M_impl._M_size)
      _M_impl = __other._M_impl._M_clone(_M_alloc);
  }

  basic_stacktrace(basic_stacktrace &&__other,
                   const allocator_type &__alloc) noexcept
      : _M_alloc(__alloc) {
    if constexpr (_Allocator::is_always_equal::value)
      _M_impl = std::exchange(__other._M_impl, {});
    else if (_M_alloc == __other._M_alloc)
      _M_impl = std::exchange(__other._M_impl, {});
    else if (const auto __s = __other._M_impl._M_size)
      _M_impl = __other._M_impl._M_clone(_M_alloc);
  }

  basic_stacktrace &operator=(const basic_stacktrace &__other) noexcept {
    if (std::addressof(__other) == this)
      return *this;

    constexpr bool __pocca =
        _AllocTraits::propagate_on_container_copy_assignment::value;
    constexpr bool __always_eq = _AllocTraits::is_always_equal::value;

    const auto __s = __other.size();

    if constexpr (!__always_eq && __pocca) {
      if (_M_alloc != __other._M_alloc) {
        // Cannot keep the same storage, so deallocate it now.
        _M_clear();
      }
    }

    if (_M_impl._M_capacity < __s) {
      // Need to allocate new storage.
      _M_clear();

      if constexpr (__pocca)
        _M_alloc = __other._M_alloc;

      _M_impl = __other._M_impl._M_clone(_M_alloc);
    } else {
      // Current storage is large enough.
      _M_impl._M_resize(0, _M_alloc);
      _M_impl._M_assign(__other._M_impl, _M_alloc);

      if constexpr (__pocca)
        _M_alloc = __other._M_alloc;
    }

    return *this;
  }

  basic_stacktrace &operator=(basic_stacktrace &&__other) noexcept {
    if (std::addressof(__other) == this)
      return *this;

    constexpr bool __pocma =
        _AllocTraits::propagate_on_container_move_assignment::value;

    if constexpr (_AllocTraits::is_always_equal::value)
      std::swap(_M_impl, __other._M_impl);
    else if (_M_alloc == __other._M_alloc)
      std::swap(_M_impl, __other._M_impl);
    else if constexpr (__pocma) {
      // Free current storage and take ownership of __other's storage.
      _M_clear();
      _M_impl = std::exchange(__other._M_impl, {});
    } else // Allocators are unequal and don't propagate.
    {
      const size_type __s = __other.size();

      if (_M_impl._M_capacity < __s) {
        // Need to allocate new storage.
        _M_clear();
        _M_impl = __other._M_impl._M_clone(_M_alloc);
      } else {
        // Current storage is large enough.
        _M_impl._M_resize(0, _M_alloc);
        _M_impl._M_assign(__other._M_impl, _M_alloc);
      }
    }

    if constexpr (__pocma)
      _M_alloc = std::move(__other._M_alloc);

    return *this;
  }

  ~basic_stacktrace() { _M_clear(); }

  // [stacktrace.basic.obs], observers
  allocator_type get_allocator() const noexcept { return _M_alloc; }

  const_iterator begin() const noexcept {
    return const_iterator{_M_impl._M_frames};
  }

  const_iterator end() const noexcept { return begin() + size(); }

  const_reverse_iterator rbegin() const noexcept {
    return std::make_reverse_iterator(end());
  }

  const_reverse_iterator rend() const noexcept {
    return std::make_reverse_iterator(begin());
  }

  const_iterator cbegin() const noexcept { return begin(); }
  const_iterator cend() const noexcept { return end(); }
  const_reverse_iterator crbegin() const noexcept { return rbegin(); };
  const_reverse_iterator crend() const noexcept { return rend(); };

  [[nodiscard]] bool empty() const noexcept { return size() == 0; }
  size_type size() const noexcept { return _M_impl._M_size; }

  size_type max_size() const noexcept {
    return _Impl::_S_max_size(_M_impl._M_alloc);
  }

  const_reference operator[](size_type __n) const noexcept {
    _FBBE_ASSERT(__n < size());
    return begin()[__n];
  }

  const_reference at(size_type __n) const {
    if (__n >= size())
      throw std::out_of_range("basic_stack_trace::at: bad frame number");
    return begin()[__n];
  }

  // [stacktrace.basic.cmp], comparisons
  template <typename _Allocator2>
  friend bool operator==(const basic_stacktrace &__x,
                         const basic_stacktrace<_Allocator2> &__y) noexcept {
    return std::equal(__x.begin(), __x.end(), __y.begin(), __y.end());
  }

#if _HAS_CXX20

  template <typename _Allocator2>
  [[nodiscard]] friend std::strong_ordering
  operator<=>(const basic_stacktrace &__x,
              const basic_stacktrace<_Allocator2> &__y) noexcept {
    if (auto __s = __x.size() <=> __y.size(); __s != 0)
      return __s;
    return std::lexicographical_compare_three_way(__x.begin(), __x.end(),
                                                  __y.begin(), __y.end());
  }
#else
  template <class _Allocator2>
  [[nodiscard]] friend int do_not_use_compare_three_way(
      const basic_stacktrace &_Lhs,
      const basic_stacktrace<_Allocator2> &_Rhs) noexcept {
    static constexpr auto int_equal = 0;
    static constexpr auto strong_three_way_comparator = [](auto lhs,
                                                           auto rhs) noexcept {
      return lhs < rhs ? -1 : (lhs == rhs ? 0 : 1);
    };

    const auto _Result =
        strong_three_way_comparator(_Lhs._Frames.size(), _Rhs._Frames.size());
    if (_Result != int_equal) {
      return _Result;
    }

    auto ix = _Lhs.begin();
    auto const ex = _Lhs.end();
    auto iy = _Rhs.begin();
    auto const ey = _Rhs.end();
    for (; ix != ex && iy != ey; ++ix, ++iy) {
      if (*ix != *iy) {
        return strong_three_way_comparator(*ix, *iy);
      }
    }

    return int_equal;
  }

  template <class _Allocator2>
  [[nodiscard]] friend bool
  operator<(const basic_stacktrace &_Lhs,
            const basic_stacktrace<_Allocator2> &_Rhs) noexcept {
    return do_not_use_compare_three_way(_Lhs, _Rhs) < 0;
  }

  template <class _Allocator2>
  [[nodiscard]] friend bool
  operator>(const basic_stacktrace &_Lhs,
            const basic_stacktrace<_Allocator2> &_Rhs) noexcept {
    return do_not_use_compare_three_way(_Lhs, _Rhs) > 0;
  }

  template <class _Allocator2>
  [[nodiscard]] friend bool
  operator<=(const basic_stacktrace &_Lhs,
             const basic_stacktrace<_Allocator2> &_Rhs) noexcept {
    return do_not_use_compare_three_way(_Lhs, _Rhs) <= 0;
  }

  template <class _Allocator2>
  [[nodiscard]] friend bool
  operator>=(const basic_stacktrace &_Lhs,
             const basic_stacktrace<_Allocator2> &_Rhs) noexcept {
    return do_not_use_compare_three_way(_Lhs, _Rhs) >= 0;
  }

  template <class _Allocator2>
  [[nodiscard]] friend bool
  operator!=(const basic_stacktrace &_Lhs,
             const basic_stacktrace<_Allocator2> &_Rhs) noexcept {
    return do_not_use_compare_three_way(_Lhs, _Rhs) != 0;
  }

#endif // _HAS_CXX20

  // [stacktrace.basic.mod], modifiers
  void swap(basic_stacktrace &__other) noexcept {
    std::swap(_M_impl, __other._M_impl);
    if constexpr (_AllocTraits::propagate_on_container_swap::value)
      std::swap(_M_alloc, __other._M_alloc);
    else if constexpr (!_AllocTraits::is_always_equal::value) {
      _FBBE_ASSERT(_M_alloc == __other._M_alloc);
    }
  }

private:
  bool _M_push_back(const value_type &__x) noexcept {
    return _M_impl._M_push_back(_M_alloc, __x);
  }

  void _M_clear() noexcept {
    _M_impl._M_resize(0, _M_alloc);
    _M_impl._M_deallocate(_M_alloc);
  }

  // Precondition: __max_depth != 0
  auto _M_prepare(size_type __max_depth = -1) noexcept
      -> int (*)(void *, uintptr_t) {
    auto __cb = +[](void *__data, uintptr_t __pc) {
      auto &__s = *static_cast<basic_stacktrace *>(__data);
      stacktrace_entry __f;
      __f._M_pc = __pc;
      if (__s._M_push_back(__f)) [[likely]]
        return 0; // continue tracing
      return -1;  // stop tracing due to error
    };

    if (__max_depth > 128)
      __max_depth = 64; // soft limit, _M_push_back will reallocate
    else
      __cb = [](void *__data, uintptr_t __pc) {
        auto &__s = *static_cast<basic_stacktrace *>(__data);
        stacktrace_entry __f;
        __f._M_pc = __pc;
        if (__s.size() == __s._M_impl._M_capacity) [[unlikely]]
          return 1; // stop tracing due to reaching max depth
        if (__s._M_push_back(__f)) [[likely]]
          return 0; // continue tracing
        return -1;  // stop tracing due to error
      };

    if (_M_impl._M_allocate(_M_alloc, __max_depth)) [[likely]]
      return __cb;
    return nullptr;
  }

  struct _Impl {
    using pointer = typename _AllocTraits::pointer;

    pointer _M_frames = nullptr;
    size_type _M_size = 0;
    size_type _M_capacity = 0;

    static size_type _S_max_size(const allocator_type &__alloc) noexcept {
      const size_t __size_max = std::numeric_limits<size_type>::max();
      const size_t __alloc_max = _AllocTraits::max_size(__alloc);
      return std::min(__size_max, __alloc_max);
    }

#if __has_builtin(__builtin_operator_new) >= 201802L
#define _FBBE_GNU_OPERATOR_NEW __builtin_operator_new
#define _FBBE_GNU_OPERATOR_DELETE __builtin_operator_delete
#else
#define _FBBE_GNU_OPERATOR_NEW ::operator new
#define _FBBE_GNU_OPERATOR_DELETE ::operator delete
#endif

    // Precondition: _M_frames == nullptr && __n != 0
    pointer _M_allocate(allocator_type &__alloc, size_type __n) noexcept {
      if (__n <= _S_max_size(__alloc)) [[likely]] {
        if constexpr (std::is_same_v<allocator_type,
                                     std::allocator<value_type>>) {
          // For std::allocator we use nothrow-new directly so we
          // don't need to handle bad_alloc exceptions.
          size_t __nb = __n * sizeof(value_type);
          void *const __p = _FBBE_GNU_OPERATOR_NEW(__nb, std::nothrow_t{});
          if (__p == nullptr) [[unlikely]]
            return nullptr;
          _M_frames = static_cast<pointer>(__p);
        } else {
          _FBBE_TRY {
            _M_frames = __alloc.allocate(__n);
          }
          _FBBE_CATCH(const std::bad_alloc &) { return nullptr; }
        }
        _M_capacity = __n;
        return _M_frames;
      }
      return nullptr;
    }

    void _M_deallocate(allocator_type &__alloc) noexcept {
      if (_M_capacity) {
        if constexpr (std::is_same_v<allocator_type,
                                     std::allocator<value_type>>)
#if defined(__cpp_sized_deallocation) && __cpp_sized_deallocation >= 201309L
          _FBBE_GNU_OPERATOR_DELETE(static_cast<void *>(_M_frames),
                                    _M_capacity * sizeof(value_type));
#else
          _FBBE_GNU_OPERATOR_DELETE(static_cast<void *>(_M_frames));                          
#endif
        else
          __alloc.deallocate(_M_frames, _M_capacity);
        _M_frames = nullptr;
        _M_capacity = 0;
      }
    }

#undef _FBBE_GNU_OPERATOR_DELETE
#undef _FBBE_GNU_OPERATOR_NEW

    // Precondition: __n <= _M_size
    void _M_resize(size_type __n, allocator_type &__alloc) noexcept {
      for (size_type __i = __n; __i < _M_size; ++__i)
        _AllocTraits::destroy(__alloc, &_M_frames[__i]);
      _M_size = __n;
    }

#if not(defined(__cpp_lib_to_address) && __cpp_lib_to_address >= 201711L)

    template <class T> constexpr T *to_address(T *p) noexcept {
      static_assert(!std::is_function_v<T>);
      return p;
    }

    template <class T> constexpr auto to_address(const T &p) noexcept {
      return to_address(p.operator->());
    }

#else
    template <class T>
    auto to_address(T &&arg)
        -> decltype(std::to_address(std::forward<T>(arg))) {
      return std::to_address(std::forward<T>(arg));
    }

#endif

    bool _M_push_back(allocator_type &__alloc,
                      const stacktrace_entry &__f) noexcept {
      if (_M_size == _M_capacity) [[unlikely]] {
        _Impl __tmp = _M_xclone(_M_capacity ? _M_capacity : 8, __alloc);
        if (!__tmp._M_capacity) [[unlikely]]
          return false;
        _M_resize(0, __alloc);
        _M_deallocate(__alloc);
        *this = __tmp;
      }
      stacktrace_entry *__addr = to_address(_M_frames + _M_size++);
      _AllocTraits::construct(__alloc, __addr, __f);
      return true;
    }

    // Precondition: _M_size != 0
    _Impl _M_clone(allocator_type &__alloc) const noexcept {
      return _M_xclone(_M_size, __alloc);
    }

    // Precondition: _M_size != 0 || __extra != 0
    _Impl _M_xclone(size_type __extra, allocator_type &__alloc) const noexcept {
      _Impl __i;
      if (__i._M_allocate(__alloc, _M_size + __extra)) [[likely]]
        __i._M_assign(*this, __alloc);
      return __i;
    }

    // Precondition: _M_capacity >= __other._M_size
    void _M_assign(const _Impl &__other, allocator_type &__alloc) noexcept {
      std::uninitialized_copy(__other._M_frames,
                              __other._M_frames + __other._M_size, _M_frames);
      _M_size = __other._M_size;
    }
  };

  [[no_unique_address]] allocator_type _M_alloc{};

  _Impl _M_impl{};
};

// basic_stacktrace typedef names
using stacktrace = basic_stacktrace<std::allocator<stacktrace_entry>>;

// [stacktrace.basic.nonmem], non-member functions
template <typename _Allocator>
inline void
swap(basic_stacktrace<_Allocator> &__a,
     basic_stacktrace<_Allocator> &__b) noexcept(noexcept(__a.swap(__b))) {
  __a.swap(__b);
}

inline std::ostream &operator<<(std::ostream &__os,
                                const stacktrace_entry &__f) {
  std::string __desc, __file;
  int __line;
  if (__f._M_get_info(&__desc, &__file, &__line)) {
    __os.width(4);
    __os << __desc << " at " << __file << ':' << __line;
  }
  return __os;
}

template <typename _Allocator>
inline std::ostream &operator<<(std::ostream &__os,
                                const basic_stacktrace<_Allocator> &__st) {
  for (stacktrace::size_type __i = 0; __i < __st.size(); ++__i) {
    __os.width(4);
    __os << __i << "# " << __st[__i] << '\n';
  }
  return __os;
}

inline std::string to_string(const stacktrace_entry &__f) {
  std::ostringstream __os;
  __os << __f;
  return std::move(__os).str();
}

template <typename _Allocator>
std::string to_string(const basic_stacktrace<_Allocator> &__st) {
  std::ostringstream __os;
  __os << __st;
  return std::move(__os).str();
}

} // namespace fbbe

#if __has_include(<memory_resource>)
#include <memory_resource>

namespace fbbe::pmr {
using stacktrace =
    basic_stacktrace<std::pmr::polymorphic_allocator<stacktrace_entry>>;
} // namespace fbbe::pmr

#endif // __has_include(<memory_resource>)

// [stacktrace.basic.hash], hash support

template <> struct std::hash<fbbe::stacktrace_entry> {
  size_t operator()(const fbbe::stacktrace_entry &__f) const noexcept {
    using __h = hash<fbbe::stacktrace_entry::native_handle_type>;
    return __h()(__f.native_handle());
  }
};

template <typename _Allocator>
struct std::hash<fbbe::basic_stacktrace<_Allocator>> {
  size_t
  operator()(const fbbe::basic_stacktrace<_Allocator> &__st) const noexcept {
    constexpr static auto hcomb = [](size_t seed, size_t value) {
      return seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    };
    hash<fbbe::stacktrace_entry::native_handle_type> __h;
    size_t __val = std::hash<size_t>{}(__st.size());
    for (const auto &__f : __st)
      __val = hcomb(__h(__f), __val);
    return __val;
  }
};
#endif
