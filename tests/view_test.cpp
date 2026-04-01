#include <gtest/gtest.h>
#include <elfio/elfio.hpp>

#include <cstring>
#include <string>
#include <vector>

using namespace elfio;

// Helper: build a minimal valid ELF64 LSB file in memory
static std::vector<char> build_minimal_elf64() {
    elf_editor<elf64_traits> ed;
    ed.create(ELFDATA2LSB, ET_EXEC, EM_X86_64);
    ed.set_entry(0x400000);

    unsigned char code[] = { 0x90, 0xc3 };  // NOP, RET
    auto& text = ed.add_section(".text", SHT_PROGBITS, SHF_ALLOC | SHF_EXECINSTR);
    text.set_data(reinterpret_cast<const char*>(code), sizeof(code));
    text.set_addr_align(16);
    text.set_address(0x400000);

    const char data[] = "test data";
    auto& data_sec = ed.add_section(".data", SHT_PROGBITS, SHF_ALLOC | SHF_WRITE);
    data_sec.set_data(data, sizeof(data));
    data_sec.set_addr_align(4);

    auto result = ed.save();
    if (!result) return {};
    return std::move(*result);
}

TEST(ViewTest, ParseMinimalElf64) {
    auto buf = build_minimal_elf64();
    ASSERT_FALSE(buf.empty());

    byte_view view{buf.data(), buf.size()};
    auto file = elf_file<elf64_traits>::from_view(view);
    ASSERT_TRUE(file.has_value());

    EXPECT_EQ(file->file_class(), ELFCLASS64);
    EXPECT_EQ(file->encoding(), ELFDATA2LSB);
    EXPECT_EQ(file->type(), ET_EXEC);
    EXPECT_EQ(file->machine(), EM_X86_64);
    EXPECT_EQ(file->entry(), 0x400000u);
    EXPECT_GE(file->section_count(), 3);
}

TEST(ViewTest, SectionIteration) {
    auto buf = build_minimal_elf64();
    ASSERT_FALSE(buf.empty());

    byte_view view{buf.data(), buf.size()};
    auto file = elf_file<elf64_traits>::from_view(view);
    ASSERT_TRUE(file.has_value());

    bool found_text = false, found_data = false;
    for (auto sec : file->sections()) {
        if (sec.name() == ".text") {
            found_text = true;
            EXPECT_TRUE(sec.is_execinstr());
            EXPECT_TRUE(sec.is_alloc());
            EXPECT_EQ(sec.size(), 2u);
        }
        if (sec.name() == ".data") {
            found_data = true;
            EXPECT_TRUE(sec.is_write());
            EXPECT_TRUE(sec.is_alloc());
        }
    }
    EXPECT_TRUE(found_text);
    EXPECT_TRUE(found_data);
}

TEST(ViewTest, SectionDataRead) {
    auto buf = build_minimal_elf64();
    ASSERT_FALSE(buf.empty());

    byte_view view{buf.data(), buf.size()};
    auto file = elf_file<elf64_traits>::from_view(view);
    ASSERT_TRUE(file.has_value());

    auto text_sec = file->find_section(".text");
    ASSERT_TRUE(text_sec.has_value());

    auto data = text_sec->data();
    ASSERT_EQ(data.size(), 2u);
    EXPECT_EQ(data[0], std::byte{0x90});
    EXPECT_EQ(data[1], std::byte{0xc3});
}

TEST(ViewTest, AutoLoadDetectsElf64) {
    auto buf = build_minimal_elf64();
    ASSERT_FALSE(buf.empty());

    byte_view view{buf.data(), buf.size()};
    auto result = auto_load(view);
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(std::holds_alternative<elf_file<elf64_traits>>(*result));
}

TEST(ViewTest, DetectElfArch) {
    auto buf = build_minimal_elf64();
    ASSERT_FALSE(buf.empty());

    byte_view view{buf.data(), buf.size()};
    auto arch = detect_elf_arch(view);
    ASSERT_TRUE(arch.has_value());
    EXPECT_EQ(*arch, detected_arch::elf64_lsb);
}

TEST(ViewTest, InvalidMagicRejected) {
    char bad_data[64] = {};
    byte_view view{bad_data, sizeof(bad_data)};
    auto file = elf_file<elf64_traits>::from_view(view);
    EXPECT_FALSE(file.has_value());
    EXPECT_EQ(file.error(), error_code::invalid_magic);
}

TEST(ViewTest, TooSmallRejected) {
    char tiny[4] = {};
    byte_view view{tiny, sizeof(tiny)};
    auto file = elf_file<elf64_traits>::from_view(view);
    EXPECT_FALSE(file.has_value());
    EXPECT_EQ(file.error(), error_code::file_too_small);
}

TEST(ViewTest, SectionFilterWithPipes) {
    auto buf = build_minimal_elf64();
    ASSERT_FALSE(buf.empty());

    byte_view view{buf.data(), buf.size()};
    auto file = elf_file<elf64_traits>::from_view(view);
    ASSERT_TRUE(file.has_value());

    std::vector<std::string> alloc_names;
    for (auto sec : file->sections()
                    | filter([](auto s) { return s.is_alloc(); })) {
        alloc_names.emplace_back(sec.name());
    }

    EXPECT_GE(alloc_names.size(), 2u);
}

TEST(ViewTest, StringTableView) {
    // Build a simple string table: \0hello\0world\0
    char strtab_data[] = "\0hello\0world\0";
    byte_view bv{strtab_data, sizeof(strtab_data) - 1};
    string_table_view stv{bv};

    EXPECT_EQ(stv.get(0), "");
    EXPECT_EQ(stv.get(1), "hello");
    EXPECT_EQ(stv.get(7), "world");
    EXPECT_EQ(stv.get(100), "");  // out of bounds → empty
}
