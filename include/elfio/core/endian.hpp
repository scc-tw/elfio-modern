#pragma once

#include <cstdint>
#include <type_traits>

namespace elfio {

enum class byte_order { little, big };

// Compile-time native byte-order detection
namespace detail {
constexpr byte_order detect_native() noexcept {
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    return byte_order::big;
#else
    return byte_order::little;
#endif
}
} // namespace detail

inline constexpr byte_order native_byte_order = detail::detect_native();

// ---------- Byte-swap overloads ----------

constexpr uint8_t  byte_swap(uint8_t  v) noexcept { return v; }

constexpr uint16_t byte_swap(uint16_t v) noexcept {
    return static_cast<uint16_t>((v >> 8) | (v << 8));
}

constexpr uint32_t byte_swap(uint32_t v) noexcept {
    return ((v >> 24) & 0x000000FFu) |
           ((v >>  8) & 0x0000FF00u) |
           ((v <<  8) & 0x00FF0000u) |
           ((v << 24) & 0xFF000000u);
}

constexpr uint64_t byte_swap(uint64_t v) noexcept {
    return ((v >> 56) & 0x00000000000000FFull) |
           ((v >> 40) & 0x000000000000FF00ull) |
           ((v >> 24) & 0x0000000000FF0000ull) |
           ((v >>  8) & 0x00000000FF000000ull) |
           ((v <<  8) & 0x000000FF00000000ull) |
           ((v << 24) & 0x0000FF0000000000ull) |
           ((v << 40) & 0x00FF000000000000ull) |
           ((v << 56) & 0xFF00000000000000ull);
}

// Signed overloads delegate to unsigned
constexpr int8_t  byte_swap(int8_t  v) noexcept { return v; }
constexpr int16_t byte_swap(int16_t v) noexcept { return static_cast<int16_t>(byte_swap(static_cast<uint16_t>(v))); }
constexpr int32_t byte_swap(int32_t v) noexcept { return static_cast<int32_t>(byte_swap(static_cast<uint32_t>(v))); }
constexpr int64_t byte_swap(int64_t v) noexcept { return static_cast<int64_t>(byte_swap(static_cast<uint64_t>(v))); }

// ---------- Conversion helpers ----------

template <typename T>
constexpr T to_native(T value, byte_order source) noexcept {
    return (source == native_byte_order) ? value : byte_swap(value);
}

template <typename T>
constexpr T from_native(T value, byte_order target) noexcept {
    return (target == native_byte_order) ? value : byte_swap(value);
}

// ---------- Endian-aware value wrapper ----------

template <typename T, byte_order Order>
struct endian_val {
    T raw;
    constexpr T    get() const noexcept { return to_native(raw, Order); }
    constexpr void set(T v) noexcept    { raw = from_native(v, Order); }
    explicit constexpr operator T() const noexcept { return get(); }
    constexpr endian_val& operator=(T v) noexcept { set(v); return *this; }
};

template <typename T> using le = endian_val<T, byte_order::little>;
template <typename T> using be = endian_val<T, byte_order::big>;

/// Runtime endianness convertor — stores the file's byte order and converts
/// fields on read/write. Used by views and editor when the encoding is
/// determined at load time rather than compile time.
class endian_convertor {
public:
    constexpr endian_convertor() noexcept = default;
    explicit constexpr endian_convertor(byte_order file_order) noexcept
        : order_(file_order), need_swap_(file_order != native_byte_order) {}

    constexpr void setup(byte_order file_order) noexcept {
        order_     = file_order;
        need_swap_ = (file_order != native_byte_order);
    }

    [[nodiscard]] constexpr byte_order order() const noexcept { return order_; }
    [[nodiscard]] constexpr bool needs_swap()  const noexcept { return need_swap_; }

    template <typename T>
    [[nodiscard]] constexpr T operator()(T value) const noexcept {
        return need_swap_ ? byte_swap(value) : value;
    }

private:
    byte_order order_     = byte_order::little;
    bool       need_swap_ = false;
};

} // namespace elfio
