#include <gtest/gtest.h>
#include <elfio/elfio.hpp>

#include <cstring>

using namespace elfio;

TEST(EditorTest, CreateAndSaveEmpty) {
    elf_editor<elf64_traits> ed;
    ed.create(ELFDATA2LSB, ET_REL, EM_X86_64);

    auto result = ed.save();
    ASSERT_TRUE(result.has_value());
    EXPECT_GT(result->size(), sizeof(Elf64_Ehdr));

    byte_view view{result->data(), result->size()};
    auto file = elf_file<elf64_traits>::from_view(view);
    ASSERT_TRUE(file.has_value());
    EXPECT_EQ(file->type(), ET_REL);
    EXPECT_EQ(file->machine(), EM_X86_64);
}

TEST(EditorTest, AddSections) {
    elf_editor<elf64_traits> ed;
    ed.create(ELFDATA2LSB, ET_REL, EM_X86_64);

    unsigned char code[] = { 0x55, 0x48, 0x89, 0xe5, 0xc3 };
    auto& text = ed.add_section(".text", SHT_PROGBITS, SHF_ALLOC | SHF_EXECINSTR);
    text.set_data(reinterpret_cast<const char*>(code), sizeof(code));
    text.set_addr_align(16);

    auto result = ed.save();
    ASSERT_TRUE(result.has_value());

    byte_view view{result->data(), result->size()};
    auto file = elf_file<elf64_traits>::from_view(view);
    ASSERT_TRUE(file.has_value());

    auto sec = file->find_section(".text");
    ASSERT_TRUE(sec.has_value());
    EXPECT_TRUE(sec->is_execinstr());
    EXPECT_TRUE(sec->is_alloc());
    EXPECT_EQ(sec->size(), sizeof(code));

    auto data = sec->data();
    EXPECT_EQ(data[0], std::byte{0x55});
    EXPECT_EQ(data[4], std::byte{0xc3});
}

TEST(EditorTest, Roundtrip32Bit) {
    elf_editor<elf32_traits> ed;
    ed.create(ELFDATA2LSB, ET_REL, EM_386);
    ed.set_os_abi(ELFOSABI_LINUX);

    const char data[] = "hello world";
    auto& sec = ed.add_section(".rodata", SHT_PROGBITS, SHF_ALLOC);
    sec.set_data(data, sizeof(data));
    sec.set_addr_align(4);

    auto result = ed.save();
    ASSERT_TRUE(result.has_value());

    byte_view view{result->data(), result->size()};
    auto file = elf_file<elf32_traits>::from_view(view);
    ASSERT_TRUE(file.has_value());
    EXPECT_EQ(file->file_class(), ELFCLASS32);
    EXPECT_EQ(file->type(), ET_REL);
    EXPECT_EQ(file->machine(), EM_386);

    auto rodata = file->find_section(".rodata");
    ASSERT_TRUE(rodata.has_value());
    EXPECT_EQ(rodata->size(), sizeof(data));

    auto sec_data = rodata->data();
    EXPECT_EQ(sec_data.read_cstring(0), "hello world");
}

TEST(EditorTest, BigEndian) {
    elf_editor<elf64_traits> ed;
    ed.create(ELFDATA2MSB, ET_REL, EM_PPC64);

    const char data[] = "big endian test";
    auto& sec = ed.add_section(".data", SHT_PROGBITS, SHF_ALLOC | SHF_WRITE);
    sec.set_data(data, sizeof(data));

    auto result = ed.save();
    ASSERT_TRUE(result.has_value());

    byte_view view{result->data(), result->size()};
    auto file = elf_file<elf64_traits>::from_view(view);
    ASSERT_TRUE(file.has_value());
    EXPECT_EQ(file->encoding(), ELFDATA2MSB);
    EXPECT_EQ(file->machine(), EM_PPC64);
}

TEST(EditorTest, StringTableBuilderDedup) {
    string_table_builder stb;
    auto idx1 = stb.add("hello");
    auto idx2 = stb.add("world");
    auto idx3 = stb.add("hello");

    EXPECT_EQ(idx1, idx3);
    EXPECT_NE(idx1, idx2);
    EXPECT_EQ(stb.add(""), 0u);
}

TEST(EditorTest, SymbolBuilder) {
    string_table_builder strtab;
    symbol_builder<elf64_traits> syms;

    (void)syms.add("main", 0x1000, 100, STB_GLOBAL, STT_FUNC, STV_DEFAULT, 1);
    (void)syms.add("data_var", 0x2000, 8, STB_GLOBAL, STT_OBJECT, STV_DEFAULT, 2);

    EXPECT_EQ(syms.count(), 3u);  // null + 2 symbols

    endian_convertor conv(byte_order::little);
    auto data = syms.build(strtab, conv);

    EXPECT_EQ(data.size(), 3u * sizeof(Elf64_Sym));
}

TEST(EditorTest, MultipleSections) {
    elf_editor<elf64_traits> ed;
    ed.create(ELFDATA2LSB, ET_REL, EM_X86_64);

    for (int i = 0; i < 10; ++i) {
        std::string name = ".sect" + std::to_string(i);
        auto& sec = ed.add_section(name, SHT_PROGBITS, SHF_ALLOC);
        char buf[32];
        std::snprintf(buf, sizeof(buf), "data for section %d", i);
        sec.set_data(buf, std::strlen(buf) + 1);
    }

    auto result = ed.save();
    ASSERT_TRUE(result.has_value());

    byte_view view{result->data(), result->size()};
    auto file = elf_file<elf64_traits>::from_view(view);
    ASSERT_TRUE(file.has_value());

    // null + 10 user + .shstrtab = 12
    EXPECT_EQ(file->section_count(), 12);

    for (int i = 0; i < 10; ++i) {
        std::string name = ".sect" + std::to_string(i);
        auto sec = file->find_section(name);
        ASSERT_TRUE(sec.has_value()) << "Missing section: " << name;
    }
}
