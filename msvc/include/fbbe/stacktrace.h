// Modified version of <stacktrace> header from MSVC STL:

// Copyright (c) Microsoft Corporation.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#pragma once
#ifndef _FBBE_STACKTRACE_
#define _FBBE_STACKTRACE_

#define _FBBE_BEGIN namespace fbbe {
#define _FBBE_END }   // namespace fbbe

#if __has_include(<stacktrace>)
#  include <stacktrace>
#  if defined(__cpp_lib_stacktrace)
#    define _FBBE_USES_STD 1
#  else
#    define _FBBE_USES_STD 0
#  endif
#else
#  define _FBBE_USES_STD 0
#endif

#if _FBBE_USES_STD

_FBBE_BEGIN
using stacktrace_entry = ::std::stacktrace_entry;
template <class _Alloc>
using basic_stacktrace = ::std::basic_stacktrace<_Alloc>;
using stacktrace = ::std::stacktrace;
namespace pmr {
using stacktrace = std::pmr::stacktrace;
}
_FBBE_END

#else

// Abi renames
#define __std_stacktrace fbbe_std_stacktrace
#define __std_stacktrace_capture fbbe_std_stacktrace_capture
#define __std_stacktrace_description fbbe_std_stacktrace_description
#define __std_stacktrace_source_file fbbe_std_stacktrace_source_file
#define __std_stacktrace_source_line fbbe_std_stacktrace_source_line
#define __std_stacktrace_address_to_string fbbe_std_stacktrace_address_to_string
#define __std_stacktrace_to_string fbbe_std_stacktrace_to_string

#define _FBBE_EXTERN_C     extern "C" {
#define _FBBE_END_EXTERN_C }

#define _EXPORT_FBBE

#include <algorithm>
#include <cstdint>
#include <memory_resource>
#include <string>
#include <type_traits>
#include <vector>

// #pragma pack(push, _CRT_PACKING)
// #pragma warning(push, _STL_WARNING_LEVEL)
// #pragma warning(disable : _STL_DISABLED_WARNINGS)
// _STL_DISABLE_CLANG_WARNINGS
// #pragma push_macro("new")
// #undef new

// The separately compiled part of the <stacktrace> implementation calls a function pointer of _Stacktrace_string_fill
// type to allocate a buffer for std::string output. The called function does the buffer allocation and calls a function
// pointer of _Stacktrace_string_fill_callback type to fill the buffer. This is needed to type-erase <std::string> or
// <ostream>, and makes it possible to keep the separately compiled implementation in the import library.
//
// Additionally, _Stacktrace_string_fill can be called with a null _Callback argument to determine the already reserved
// std::string buffer for cases when the size is not known before an attempt to fill it -- this potentially avoids both an
// extra fill attempt and an extra allocation if the reserved size is enough.

using _Stacktrace_string_fill_callback = size_t(__stdcall*)(char* _Data, size_t _Size, void* _Context) _NOEXCEPT_FNPTR;

using _Stacktrace_string_fill = size_t(__stdcall*)(
    size_t _Size, void* _String, void* _Context, _Stacktrace_string_fill_callback _Callback);

_FBBE_EXTERN_C
[[nodiscard]] unsigned short __stdcall __std_stacktrace_capture(unsigned long _Frames_to_skip,
    unsigned long _Frames_to_capture, void** _Back_trace, unsigned long* _Back_trace_hash) noexcept;

// Some of these functions may throw
// (they would propagate bad_alloc potentially thrown from std::string::resize_and_overwrite)

void __stdcall __std_stacktrace_address_to_string(
    const void* _Address, void* _Str, _Stacktrace_string_fill _Fill) noexcept(false);

void __stdcall __std_stacktrace_description( //
    const void* _Address, void* _Str, _Stacktrace_string_fill _Fill) noexcept(false);

void __stdcall __std_stacktrace_source_file( //
    const void* _Address, void* _Str, _Stacktrace_string_fill _Fill) noexcept(false);

[[nodiscard]] unsigned int __stdcall __std_stacktrace_source_line(const void* _Address) noexcept;

void __stdcall __std_stacktrace_to_string(
    const void* const* _Addresses, size_t _Size, void* _Str, _Stacktrace_string_fill _Fill) noexcept(false);
_FBBE_END_EXTERN_C

_FBBE_BEGIN

namespace detail {
    template< class Operation >
    inline void resize_and_overwrite(std::string& self, size_t count, Operation op ){
        if(count <= self.capacity()){
            self.resize(count);
            self.resize(std::move(op)(&self[0], count));      
        } else {
            std::string buff{};
            buff.reserve(count);
            auto k = (std::min)(self.size(), count);
            buff.append(self.data(), k);
            buff.resize(count); // unfortunately, this is needed to overwrite the rest of the buffer
            buff.resize(std::move(op)(&buff[0], count));
            self = std::move(buff);
        }
    }
}

inline size_t __stdcall _Stacktrace_string_fill_impl(
    const size_t _Size, void* const _String, void* const _Context, const _Stacktrace_string_fill_callback _Callback) {
    if (_Callback) {

        detail::resize_and_overwrite(*static_cast<std::string*>(_String), _Size,
            [_Callback, _Context](char* _Data, size_t _Size) noexcept { return _Callback(_Data, _Size, _Context); });
        return static_cast<std::string*>(_String)->size();
    } else {
        return static_cast<std::string*>(_String)->capacity();
    }
}

_EXPORT_FBBE class stacktrace_entry {
public:
    using native_handle_type = void*;

    constexpr stacktrace_entry() noexcept                                   = default;
    constexpr stacktrace_entry(const stacktrace_entry&) noexcept            = default;
    constexpr stacktrace_entry& operator=(const stacktrace_entry&) noexcept = default;

    ~stacktrace_entry() = default;

    [[nodiscard]] constexpr native_handle_type native_handle() const noexcept {
        return _Address;
    }

    [[nodiscard]] constexpr explicit operator bool() const noexcept {
        return _Address != nullptr;
    }

    [[nodiscard]] std::string description() const {
        std::string _Result;
        __std_stacktrace_description(_Address, &_Result, _Stacktrace_string_fill_impl);
        return _Result;
    }

    [[nodiscard]] std::string source_file() const {
        std::string _Result;
        __std_stacktrace_source_file(_Address, &_Result, _Stacktrace_string_fill_impl);
        return _Result;
    }

    [[nodiscard]] uint_least32_t source_line() const noexcept /* strengthened */ {
        return __std_stacktrace_source_line(_Address);
    }

#if _HAS_CXX20

    [[nodiscard]] friend constexpr bool operator==(const stacktrace_entry&, const stacktrace_entry&) noexcept = default;
    [[nodiscard]] friend constexpr std::strong_ordering operator<=>(const stacktrace_entry&, const stacktrace_entry&) noexcept = default;

#else

    [[nodiscard]] friend constexpr bool operator==(const stacktrace_entry& lhs, const stacktrace_entry& rhs) noexcept{
        return lhs._Address == rhs._Address;
    }

    [[nodiscard]] friend constexpr bool operator!=(const stacktrace_entry& lhs, const stacktrace_entry& rhs) noexcept{
        return lhs._Address != rhs._Address;
    }

    [[nodiscard]] friend constexpr bool operator<(const stacktrace_entry& lhs, const stacktrace_entry& rhs) noexcept{
        return lhs._Address < rhs._Address;
    }

    [[nodiscard]] friend constexpr bool operator<=(const stacktrace_entry& lhs, const stacktrace_entry& rhs) noexcept{
        return lhs._Address <= rhs._Address;
    }

    [[nodiscard]] friend constexpr bool operator>(const stacktrace_entry& lhs, const stacktrace_entry& rhs) noexcept{
        return lhs._Address > rhs._Address;
    }

    [[nodiscard]] friend constexpr bool operator>=(const stacktrace_entry& lhs, const stacktrace_entry& rhs) noexcept{
        return lhs._Address >= rhs._Address;
    }

#endif

private:
    void* _Address = nullptr;
};

_EXPORT_FBBE template <class _Alloc>
class basic_stacktrace {
private:
    using _Frames_t = std::vector<stacktrace_entry, _Alloc>;

public:
    using value_type             = stacktrace_entry;
    using const_reference        = const value_type&;
    using reference              = value_type&;
    using const_iterator         = typename _Frames_t::const_iterator;
    using iterator               = const_iterator;
    using reverse_iterator       = _STD reverse_iterator<iterator>;
    using const_reverse_iterator = _STD reverse_iterator<const_iterator>;
    using difference_type        = typename _Frames_t::difference_type;
    using size_type              = typename _Frames_t::size_type;
    using allocator_type         = _Alloc;

    // __declspec(noinline) to make the same behavior for debug and release.
    // We force the current function to be always noinline and add its frame to skipped.

    [[nodiscard]] __declspec(noinline) static basic_stacktrace
        current(const allocator_type& _Al = allocator_type()) noexcept {
        _TRY_BEGIN
        basic_stacktrace _Result{_Internal_t{}, _Max_frames, _Al};
        const unsigned short _Actual_size = __std_stacktrace_capture(
            1, static_cast<unsigned long>(_Max_frames), _Result._To_voidptr_array(), &_Result._Hash);
        _Result._Frames.resize(_Actual_size);
        return _Result;
        _CATCH_ALL
        return basic_stacktrace{_Al};
        _CATCH_END
    }

    [[nodiscard]] __declspec(noinline) static basic_stacktrace
        current(const size_type _Skip, const allocator_type& _Al = allocator_type()) noexcept {
        _TRY_BEGIN
        basic_stacktrace _Result{_Internal_t{}, _Max_frames, _Al};
        const unsigned short _Actual_size = __std_stacktrace_capture(
            _Adjust_skip(_Skip), static_cast<unsigned long>(_Max_frames), _Result._To_voidptr_array(), &_Result._Hash);
        _Result._Frames.resize(_Actual_size);
        return _Result;
        _CATCH_ALL
        return basic_stacktrace{_Al};
        _CATCH_END
    }

    [[nodiscard]] __declspec(noinline) static basic_stacktrace
        current(const size_type _Skip, size_type _Max_depth, const allocator_type& _Al = allocator_type()) noexcept {
        _TRY_BEGIN
        if (_Max_depth > _Max_frames) {
            _Max_depth = _Max_frames;
        }

        basic_stacktrace _Result{_Internal_t{}, _Max_depth, _Al};

        const unsigned short _Actual_size = __std_stacktrace_capture(
            _Adjust_skip(_Skip), static_cast<unsigned long>(_Max_depth), _Result._To_voidptr_array(), &_Result._Hash);
        _Result._Frames.resize(_Actual_size);
        return _Result;
        _CATCH_ALL
        return basic_stacktrace{_Al};
        _CATCH_END
    }

    basic_stacktrace() noexcept(std::is_nothrow_default_constructible_v<allocator_type>) = default;
    explicit basic_stacktrace(const allocator_type& _Al) noexcept : _Frames(_Al) {}

    basic_stacktrace(const basic_stacktrace&)     = default;
    basic_stacktrace(basic_stacktrace&&) noexcept = default;
    basic_stacktrace(const basic_stacktrace& _Other, const allocator_type& _Al)
        : _Frames(_Other._Frames, _Al), _Hash(_Other._Hash) {}

    basic_stacktrace(basic_stacktrace&& _Other, const allocator_type& _Al)
        : _Frames(_STD move(_Other._Frames), _Al), _Hash(_Other._Hash) {}

    basic_stacktrace& operator=(const basic_stacktrace&)                 = default;
    basic_stacktrace& operator=(basic_stacktrace&&) noexcept(_Noex_move) = default;

    ~basic_stacktrace() = default;

    [[nodiscard]] allocator_type get_allocator() const noexcept {
        return _Frames.get_allocator();
    }

    [[nodiscard]] const_iterator begin() const noexcept {
        return _Frames.cbegin();
    }

    [[nodiscard]] const_iterator end() const noexcept {
        return _Frames.cend();
    }

    [[nodiscard]] const_reverse_iterator rbegin() const noexcept {
        return _Frames.crbegin();
    }

    [[nodiscard]] const_reverse_iterator rend() const noexcept {
        return _Frames.crend();
    }

    [[nodiscard]] const_iterator cbegin() const noexcept {
        return _Frames.cbegin();
    }

    [[nodiscard]] const_iterator cend() const noexcept {
        return _Frames.cend();
    }

    [[nodiscard]] const_reverse_iterator crbegin() const noexcept {
        return _Frames.crbegin();
    }

    [[nodiscard]] const_reverse_iterator crend() const noexcept {
        return _Frames.crend();
    }

    [[nodiscard ("This function does not empty the stacktrace")]] bool empty() const noexcept {
        return _Frames.empty();
    }

    [[nodiscard]] size_type size() const noexcept {
        return _Frames.size();
    }

    [[nodiscard]] size_type max_size() const noexcept {
        return _Frames.max_size();
    }

    [[nodiscard]] const_reference operator[](const size_type _Sx) const noexcept /* strengthened */ {
        return _Frames[_Sx];
    }

    [[nodiscard]] const_reference at(const size_type _Sx) const {
        return _Frames.at(_Sx);
    }

    template <class _Al2>
    [[nodiscard]] friend bool operator==(const basic_stacktrace& _Lhs, const basic_stacktrace<_Al2>& _Rhs) noexcept {
        return _Lhs._Hash == _Rhs._Hash && ::std::equal(_Lhs.begin(), _Lhs.end(), _Rhs.begin(), _Rhs.end());
    }

#if _HAS_CXX20
    template <class _Al2>
    [[nodiscard]] friend std::strong_ordering operator<=>(
        const basic_stacktrace& _Lhs, const basic_stacktrace<_Al2>& _Rhs) noexcept {
        const auto _Result = _Lhs._Frames.size() <=> _Rhs._Frames.size();
        if (_Result != std::strong_ordering::equal) {
            return _Result;
        }

#ifdef __cpp_lib_concepts
        return _STD lexicographical_compare_three_way(_Lhs.begin(), _Lhs.end(), _Rhs.begin(), _Rhs.end());
#else // ^^^ __cpp_lib_concepts ^^^ / vvv !__cpp_lib_concepts vvv
        for (size_t _Ix = 0, _Mx = _Lhs._Frames.size(); _Ix != _Mx; ++_Ix) {
            if (_Lhs._Frames[_Ix] != _Rhs._Frames[_Ix]) {
                return _Lhs._Frames[_Ix] <=> _Rhs._Frames[_Ix];
            }
        }

        return strong_ordering::equal;
#endif // ^^^ !__cpp_lib_concepts ^^^
    }
#else
    template <class _Al2>
    [[nodiscard]] friend int do_not_use_compare_three_way(
        const basic_stacktrace& _Lhs, const basic_stacktrace<_Al2>& _Rhs) noexcept {
        static constexpr auto int_equal = 0;
        static constexpr auto strong_three_way_comparator = [](auto lhs, auto rhs) noexcept {
            return lhs < rhs ? -1 : (lhs == rhs ? 0 : 1);
        };
        
        const auto _Result = strong_three_way_comparator(_Lhs._Frames.size() , _Rhs._Frames.size());
        if (_Result != int_equal) {
            return _Result;
        }
    
        for (size_t _Ix = 0, _Mx = _Lhs._Frames.size(); _Ix != _Mx; ++_Ix) {
            if (_Lhs._Frames[_Ix] != _Rhs._Frames[_Ix]) {
                return strong_three_way_comparator(_Lhs._Frames[_Ix], _Rhs._Frames[_Ix]);
            }
        }
    
        return int_equal;
    }

    template <class _Al2>
    [[nodiscard]] friend bool operator<(const basic_stacktrace& _Lhs, const basic_stacktrace<_Al2>& _Rhs) noexcept {
        return do_not_use_compare_three_way(_Lhs, _Rhs) < 0;
    }

    template <class _Al2>
    [[nodiscard]] friend bool operator>(const basic_stacktrace& _Lhs, const basic_stacktrace<_Al2>& _Rhs) noexcept {
        return do_not_use_compare_three_way(_Lhs, _Rhs) > 0;
    }

    template <class _Al2>
    [[nodiscard]] friend bool operator<=(const basic_stacktrace& _Lhs, const basic_stacktrace<_Al2>& _Rhs) noexcept {
        return do_not_use_compare_three_way(_Lhs, _Rhs) <= 0;
    }

    template <class _Al2>
    [[nodiscard]] friend bool operator>=(const basic_stacktrace& _Lhs, const basic_stacktrace<_Al2>& _Rhs) noexcept {
        return do_not_use_compare_three_way(_Lhs, _Rhs) >= 0;
    }

    template <class _Al2>
    [[nodiscard]] friend bool operator!=(const basic_stacktrace& _Lhs, const basic_stacktrace<_Al2>& _Rhs) noexcept {
        return do_not_use_compare_three_way(_Lhs, _Rhs) != 0;
    }

#endif // _HAS_CXX20

    void swap(basic_stacktrace& _Other) noexcept(std::allocator_traits<_Alloc>::propagate_on_container_swap::value
                                                 || std::allocator_traits<_Alloc>::is_always_equal::value) {
        _Frames.swap(_Other._Frames);
        _STD swap(_Hash, _Other._Hash);
    }

    [[nodiscard]] unsigned long _Get_hash() const noexcept {
        return _Hash;
    }

    _STL_INTERNAL_STATIC_ASSERT(sizeof(void*) == sizeof(stacktrace_entry));
    _STL_INTERNAL_STATIC_ASSERT(alignof(void*) == alignof(stacktrace_entry));

    [[nodiscard]] const void* const* _To_voidptr_array() const noexcept {
        return reinterpret_cast<const void* const*>(_Frames.data());
    }

    [[nodiscard]] void** _To_voidptr_array() noexcept {
        return reinterpret_cast<void**>(_Frames.data());
    }

private:
    static constexpr size_t _Max_frames = 0xFFFF;

    static constexpr bool _Noex_move = std::allocator_traits<_Alloc>::propagate_on_container_move_assignment::value
                                    || std::allocator_traits<_Alloc>::is_always_equal::value;

    struct _Internal_t {
        explicit _Internal_t() noexcept = default;
    };

    basic_stacktrace(_Internal_t, size_type _Max_depth, const allocator_type& _Al) : _Frames(_Max_depth, _Al) {}

    [[nodiscard]] static unsigned long _Adjust_skip(size_t _Skip) noexcept {
        return _Skip < ULONG_MAX - 1 ? static_cast<unsigned long>(_Skip + 1) : ULONG_MAX;
    }

    _Frames_t _Frames;
    unsigned long _Hash = 0;
};

_EXPORT_FBBE using stacktrace = basic_stacktrace<std::allocator<stacktrace_entry>>;

_EXPORT_FBBE template <class _Alloc>
void swap(basic_stacktrace<_Alloc>& _Ax, basic_stacktrace<_Alloc>& _Bx) noexcept(noexcept(_Ax.swap(_Bx))) {
    _Ax.swap(_Bx);
}

_EXPORT_FBBE [[nodiscard]] inline std::string to_string(const stacktrace_entry& _Fx) {
    std::string _Result;
    __std_stacktrace_address_to_string(_Fx.native_handle(), &_Result, _Stacktrace_string_fill_impl);
    return _Result;
}

_EXPORT_FBBE template <class _Alloc>
[[nodiscard]] std::string to_string(const basic_stacktrace<_Alloc>& _St) {
    std::string _Result;
    __std_stacktrace_to_string(_St._To_voidptr_array(), _St.size(), &_Result, _Stacktrace_string_fill_impl);
    return _Result;
}

_EXPORT_FBBE template <class _CharT, class _Traits>
std::basic_ostream<_CharT, _Traits>& operator<<(std::basic_ostream<_CharT, _Traits>& _Os, const stacktrace_entry& _Fx) {
    return _Os << ::fbbe::to_string(_Fx);
}

_EXPORT_FBBE template <class _CharT, class _Traits, class _Alloc>
std::basic_ostream<_CharT, _Traits>& operator<<(std::basic_ostream<_CharT, _Traits>& _Os, const basic_stacktrace<_Alloc>& _St) {
    return _Os << ::fbbe::to_string(_St);
}

namespace pmr {
    _EXPORT_FBBE using stacktrace = basic_stacktrace<std::pmr::polymorphic_allocator<stacktrace_entry>>;
}

_FBBE_END

template <>
struct std::hash<fbbe::stacktrace_entry> {
    // This is a C++23 feature, so argument_type and result_type are omitted.

    [[nodiscard]] size_t operator()(const fbbe::stacktrace_entry& _Val) const noexcept {
        return _Hash_representation(_Val.native_handle());
    }
};

template <class _Alloc>
struct std::hash<fbbe::basic_stacktrace<_Alloc>> {
    // This is a C++23 feature, so argument_type and result_type are omitted.

    [[nodiscard]] size_t operator()(const fbbe::basic_stacktrace<_Alloc>& _Val) const noexcept {
        return _Val._Get_hash();
    }
};

// #pragma pop_macro("new")
// _STL_RESTORE_CLANG_WARNINGS
// #pragma warning(pop)
// #pragma pack(pop)

#undef __std_stacktrace_capture
#undef __std_stacktrace_description
#undef __std_stacktrace_source_file
#undef __std_stacktrace_source_line
#undef __std_stacktrace_address_to_string
#undef __std_stacktrace_to_string

#endif // _FBBE_USES_STD 0
#endif // _FBBE_STACKTRACE_