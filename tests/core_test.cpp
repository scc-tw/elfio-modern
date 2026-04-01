#include <gtest/gtest.h>
#include <elfio/core/error.hpp>
#include <elfio/core/result.hpp>
#include <elfio/core/safe_math.hpp>
#include <elfio/core/byte_view.hpp>
#include <elfio/core/endian.hpp>
#include <elfio/core/lazy.hpp>

#include <vector>

using namespace elfio;

// ================================================================
//  error_code tests
// ================================================================

TEST(ErrorTest, ToStringCoversAllCodes) {
    EXPECT_EQ(to_string(error_code::success), "success");
    EXPECT_EQ(to_string(error_code::out_of_bounds), "access out of bounds");
    EXPECT_EQ(to_string(error_code::invalid_magic), "invalid ELF magic number");
    EXPECT_EQ(to_string(error_code::overflow), "integer overflow");
}

// ================================================================
//  result<T, E> tests
// ================================================================

TEST(ResultTest, ValueConstruction) {
    result<int> r(42);
    EXPECT_TRUE(r.has_value());
    EXPECT_TRUE(static_cast<bool>(r));
    EXPECT_EQ(r.value(), 42);
    EXPECT_EQ(*r, 42);
}

TEST(ResultTest, ErrorConstruction) {
    result<int> r(error_code::overflow);
    EXPECT_FALSE(r.has_value());
    EXPECT_FALSE(static_cast<bool>(r));
    EXPECT_EQ(r.error(), error_code::overflow);
}

TEST(ResultTest, UnexpectedConstruction) {
    result<int> r(unexpected{error_code::out_of_bounds});
    EXPECT_FALSE(r.has_value());
    EXPECT_EQ(r.error(), error_code::out_of_bounds);
}

TEST(ResultTest, ValueOr) {
    result<int> ok(10);
    result<int> err(error_code::overflow);
    EXPECT_EQ(ok.value_or(99), 10);
    EXPECT_EQ(err.value_or(99), 99);
}

TEST(ResultTest, ArrowOperator) {
    struct S { int x; };
    result<S> r(S{7});
    EXPECT_EQ(r->x, 7);
}

TEST(ResultTest, Map) {
    result<int> r(5);
    auto doubled = r.map([](int v) { return v * 2; });
    EXPECT_TRUE(doubled.has_value());
    EXPECT_EQ(*doubled, 10);
}

TEST(ResultTest, MapPropagatesError) {
    result<int> r(error_code::overflow);
    auto doubled = r.map([](int v) { return v * 2; });
    EXPECT_FALSE(doubled.has_value());
    EXPECT_EQ(doubled.error(), error_code::overflow);
}

TEST(ResultTest, AndThen) {
    result<int> r(5);
    auto chained = r.and_then([](int v) -> result<int> {
        return v + 100;
    });
    EXPECT_TRUE(chained.has_value());
    EXPECT_EQ(*chained, 105);
}

TEST(ResultTest, VoidSpecialisation) {
    result<void> ok;
    EXPECT_TRUE(ok.has_value());

    result<void> err(error_code::invalid_magic);
    EXPECT_FALSE(err.has_value());
    EXPECT_EQ(err.error(), error_code::invalid_magic);
}

// ================================================================
//  safe_math tests
// ================================================================

TEST(SafeMathTest, CheckedAddNormal) {
    auto r = checked_add<uint32_t>(10, 20);
    ASSERT_TRUE(r);
    EXPECT_EQ(*r, 30u);
}

TEST(SafeMathTest, CheckedAddOverflow) {
    auto r = checked_add<uint32_t>(UINT32_MAX, 1);
    EXPECT_FALSE(r.has_value());
    EXPECT_EQ(r.error(), error_code::overflow);
}

TEST(SafeMathTest, CheckedMulNormal) {
    auto r = checked_mul<uint32_t>(100, 200);
    ASSERT_TRUE(r);
    EXPECT_EQ(*r, 20000u);
}

TEST(SafeMathTest, CheckedMulOverflow) {
    auto r = checked_mul<uint64_t>(UINT64_MAX, 2);
    EXPECT_FALSE(r.has_value());
}

TEST(SafeMathTest, AlignTo) {
    EXPECT_EQ(align_to<uint32_t>(0, 4), 0u);
    EXPECT_EQ(align_to<uint32_t>(1, 4), 4u);
    EXPECT_EQ(align_to<uint32_t>(4, 4), 4u);
    EXPECT_EQ(align_to<uint32_t>(5, 4), 8u);
    EXPECT_EQ(align_to<uint32_t>(7, 0), 7u);  // alignment==0 guard
}

TEST(SafeMathTest, NarrowCast) {
    auto ok = narrow_cast<uint16_t>(uint32_t(100));
    ASSERT_TRUE(ok);
    EXPECT_EQ(*ok, 100);

    auto fail = narrow_cast<uint16_t>(uint32_t(70000));
    EXPECT_FALSE(fail.has_value());
}

// ================================================================
//  byte_view tests
// ================================================================

TEST(ByteViewTest, ReadTrivial) {
    uint32_t val = 0x12345678;
    byte_view bv{&val, sizeof(val)};
    auto r = bv.read<uint32_t>(0);
    ASSERT_TRUE(r);
    EXPECT_EQ(*r, 0x12345678u);
}

TEST(ByteViewTest, OutOfBounds) {
    uint32_t val = 0;
    byte_view bv{&val, sizeof(val)};
    auto r = bv.read<uint32_t>(1);
    EXPECT_FALSE(r.has_value());
    EXPECT_EQ(r.error(), error_code::out_of_bounds);
}

TEST(ByteViewTest, Subview) {
    char data[] = "Hello, World!";
    byte_view bv{data, sizeof(data) - 1};
    auto sub = bv.subview(7, 5);
    ASSERT_TRUE(sub);
    EXPECT_EQ(sub->size(), 5u);
    EXPECT_EQ(sub->read_cstring(0), "World");
}

TEST(ByteViewTest, ReadCstring) {
    char data[] = "foo\0bar";
    byte_view bv{data, 7};
    EXPECT_EQ(bv.read_cstring(0), "foo");
    EXPECT_EQ(bv.read_cstring(4), "bar");
}

TEST(ByteViewTest, ReadArray) {
    uint32_t arr[] = {1, 2, 3, 4};
    byte_view bv{arr, sizeof(arr)};
    auto sub = bv.read_array<uint32_t>(0, 4);
    ASSERT_TRUE(sub);
    EXPECT_EQ(sub->size(), sizeof(arr));

    auto overflow = bv.read_array<uint32_t>(0, 5);
    EXPECT_FALSE(overflow.has_value());
}

// ================================================================
//  endian tests
// ================================================================

TEST(EndianTest, ByteSwap16) {
    EXPECT_EQ(byte_swap(uint16_t(0x1234)), uint16_t(0x3412));
}

TEST(EndianTest, ByteSwap32) {
    EXPECT_EQ(byte_swap(uint32_t(0x12345678)), uint32_t(0x78563412));
}

TEST(EndianTest, ConvertorNoSwap) {
    endian_convertor conv(native_byte_order);
    EXPECT_EQ(conv(uint32_t(42)), 42u);
    EXPECT_FALSE(conv.needs_swap());
}

TEST(EndianTest, ConvertorSwap) {
    auto other = (native_byte_order == byte_order::little)
                     ? byte_order::big : byte_order::little;
    endian_convertor conv(other);
    EXPECT_TRUE(conv.needs_swap());
    EXPECT_EQ(conv(uint16_t(0x1234)), uint16_t(0x3412));
}

// ================================================================
//  lazy range tests
// ================================================================

TEST(LazyTest, Filter) {
    std::vector<int> v = {1, 2, 3, 4, 5, 6};
    std::vector<int> out;
    for (auto x : v | filter([](int i) { return i % 2 == 0; }))
        out.push_back(x);
    EXPECT_EQ(out, (std::vector<int>{2, 4, 6}));
}

TEST(LazyTest, Transform) {
    std::vector<int> v = {1, 2, 3};
    std::vector<int> out;
    for (auto x : v | transform([](int i) { return i * 10; }))
        out.push_back(x);
    EXPECT_EQ(out, (std::vector<int>{10, 20, 30}));
}

TEST(LazyTest, FilterTransformPipe) {
    std::vector<int> v = {1, 2, 3, 4, 5};
    std::vector<int> out;
    for (auto x : v | filter([](int i) { return i > 2; })
                    | transform([](int i) { return i * 100; }))
        out.push_back(x);
    EXPECT_EQ(out, (std::vector<int>{300, 400, 500}));
}

TEST(LazyTest, FindFirst) {
    std::vector<int> v = {10, 20, 30, 40};
    auto found = find_first(v, [](int i) { return i > 25; });
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(*found, 30);

    auto not_found = find_first(v, [](int i) { return i > 100; });
    EXPECT_FALSE(not_found.has_value());
}
