#include <gtest/gtest.h>
#include <elfio/elfio.hpp>

#include <cstring>
#include <string>

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

// ================================================================
//  Relocation builder tests
// ================================================================

TEST(EditorTest, RelocationBuilderRel) {
    relocation_builder<elf64_traits> rb;
    rb.add_rel(0x1000, 5, R_X86_64_64);
    rb.add_rel(0x2000, 10, R_X86_64_PC32);
    EXPECT_EQ(rb.rel_count(), 2u);

    endian_convertor conv(byte_order::little);
    auto data = rb.build_rel(conv);
    EXPECT_EQ(data.size(), 2u * sizeof(Elf64_Rel));

    // Parse back
    byte_view bv{data.data(), data.size()};
    auto r0 = bv.read<Elf64_Rel>(0);
    ASSERT_TRUE(r0.has_value());
    EXPECT_EQ(conv(r0->r_offset), 0x1000u);
    EXPECT_EQ(elf64_r_sym(conv(r0->r_info)), 5u);
    EXPECT_EQ(elf64_r_type(conv(r0->r_info)), static_cast<uint64_t>(R_X86_64_64));
}

TEST(EditorTest, RelocationBuilderRela) {
    relocation_builder<elf64_traits> rb;
    rb.add_rela(0x3000, 7, R_X86_64_PLT32, -4);
    EXPECT_EQ(rb.rela_count(), 1u);

    endian_convertor conv(byte_order::little);
    auto data = rb.build_rela(conv);
    EXPECT_EQ(data.size(), sizeof(Elf64_Rela));

    byte_view bv{data.data(), data.size()};
    auto r0 = bv.read<Elf64_Rela>(0);
    ASSERT_TRUE(r0.has_value());
    EXPECT_EQ(conv(r0->r_offset), 0x3000u);
    EXPECT_EQ(conv(r0->r_addend), -4);
}

TEST(EditorTest, RelocationRoundtrip) {
    elf_editor<elf64_traits> ed;
    ed.create(ELFDATA2LSB, ET_REL, EM_X86_64);

    // .text
    unsigned char code[] = { 0x90 };
    auto& text = ed.add_section(".text", SHT_PROGBITS, SHF_ALLOC | SHF_EXECINSTR);
    text.set_data(reinterpret_cast<const char*>(code), sizeof(code));

    // Build rela data
    relocation_builder<elf64_traits> rb;
    rb.add_rela(0x0, 1, R_X86_64_64, 0);
    endian_convertor conv(byte_order::little);
    auto rela_data = rb.build_rela(conv);

    auto& rela_sec = ed.add_section(".rela.text", SHT_RELA, SHF_INFO_LINK);
    rela_sec.set_data(rela_data.data(), rela_data.size());
    rela_sec.set_entry_size(sizeof(Elf64_Rela));
    rela_sec.set_link(0); // would point to .symtab
    rela_sec.set_info(1); // .text section index

    auto result = ed.save();
    ASSERT_TRUE(result.has_value());

    byte_view view{result->data(), result->size()};
    auto file = elf_file<elf64_traits>::from_view(view);
    ASSERT_TRUE(file.has_value());

    auto sec = file->find_section(".rela.text");
    ASSERT_TRUE(sec.has_value());
    EXPECT_TRUE(sec->is_rela());
    EXPECT_EQ(sec->entry_size(), sizeof(Elf64_Rela));
}

// ================================================================
//  Dynamic builder tests
// ================================================================

TEST(EditorTest, DynamicBuilder) {
    dynamic_builder<elf64_traits> db;
    db.add_needed("libc.so.6");
    db.add_needed("libm.so.6");
    db.add(DT_FLAGS, DF_BIND_NOW);
    EXPECT_EQ(db.count(), 3u);

    string_table_builder strtab;
    endian_convertor conv(byte_order::little);
    auto data = db.build(strtab, conv);

    // 3 entries + 1 DT_NULL = 4
    EXPECT_EQ(data.size(), 4u * sizeof(Elf64_Dyn));

    // Verify strings were added to strtab
    EXPECT_GT(strtab.size(), 1u);
}

TEST(EditorTest, DynamicRoundtrip) {
    elf_editor<elf64_traits> ed;
    ed.create(ELFDATA2LSB, ET_DYN, EM_X86_64);

    string_table_builder dynstr;
    dynamic_builder<elf64_traits> db;
    db.add_needed("libtest.so");
    db.add_soname("mylib.so");

    endian_convertor conv(byte_order::little);
    auto dyn_data = db.build(dynstr, conv);

    auto& dynstr_sec = ed.add_section(".dynstr", SHT_STRTAB, SHF_ALLOC);
    dynstr_sec.set_data(dynstr.data());

    auto& dyn_sec = ed.add_section(".dynamic", SHT_DYNAMIC, SHF_ALLOC | SHF_WRITE);
    dyn_sec.set_data(dyn_data.data(), dyn_data.size());
    dyn_sec.set_entry_size(sizeof(Elf64_Dyn));
    dyn_sec.set_link(1); // .dynstr index

    auto result = ed.save();
    ASSERT_TRUE(result.has_value());

    byte_view view{result->data(), result->size()};
    auto file = elf_file<elf64_traits>::from_view(view);
    ASSERT_TRUE(file.has_value());

    auto sec = file->find_section(".dynamic");
    ASSERT_TRUE(sec.has_value());
    EXPECT_TRUE(sec->is_dynamic());
}

// ================================================================
//  Note builder tests
// ================================================================

TEST(EditorTest, NoteBuilder) {
    note_builder nb;
    std::vector<char> desc = {0x01, 0x02, 0x03, 0x04};
    nb.add("GNU", NT_GNU_BUILD_ID, desc);
    EXPECT_EQ(nb.count(), 1u);

    endian_convertor conv(byte_order::little);
    auto data = nb.build(conv);
    EXPECT_GT(data.size(), sizeof(Elf_Nhdr));

    // Parse back
    byte_view bv{data.data(), data.size()};
    note_range notes{bv, &conv};
    std::size_t count = 0;
    for (auto note : notes) {
        EXPECT_EQ(note.name(), "GNU");
        EXPECT_EQ(note.type(), NT_GNU_BUILD_ID);
        EXPECT_EQ(note.desc().size(), 4u);
        ++count;
    }
    EXPECT_EQ(count, 1u);
}

TEST(EditorTest, NoteRoundtrip) {
    elf_editor<elf64_traits> ed;
    ed.create(ELFDATA2LSB, ET_EXEC, EM_X86_64);

    note_builder nb;
    std::vector<char> build_id = {'\xDE', '\xAD', '\xBE', '\xEF'};
    nb.add("GNU", NT_GNU_BUILD_ID, build_id);

    endian_convertor conv(byte_order::little);
    auto note_data = nb.build(conv);

    auto& note_sec = ed.add_section(".note.gnu.build-id", SHT_NOTE, SHF_ALLOC);
    note_sec.set_data(note_data.data(), note_data.size());
    note_sec.set_addr_align(4);

    auto result = ed.save();
    ASSERT_TRUE(result.has_value());

    byte_view view{result->data(), result->size()};
    auto file = elf_file<elf64_traits>::from_view(view);
    ASSERT_TRUE(file.has_value());

    auto sec = file->find_section(".note.gnu.build-id");
    ASSERT_TRUE(sec.has_value());
    EXPECT_TRUE(sec->is_note());

    for (auto note : file->notes(*sec)) {
        EXPECT_EQ(note.name(), "GNU");
        EXPECT_EQ(note.type(), NT_GNU_BUILD_ID);
        EXPECT_EQ(note.desc().size(), 4u);
    }
}

// ================================================================
//  Array builder tests
// ================================================================

TEST(EditorTest, ArrayBuilder) {
    array_builder<uint64_t> ab;
    ab.add(0x1000);
    ab.add(0x2000);
    ab.add(0x3000);
    EXPECT_EQ(ab.count(), 3u);

    endian_convertor conv(byte_order::little);
    auto data = ab.build(conv);
    EXPECT_EQ(data.size(), 3u * sizeof(uint64_t));

    // Parse back
    byte_view bv{data.data(), data.size()};
    array_range<uint64_t> arr{bv, &conv};
    EXPECT_EQ(arr.size(), 3u);
    EXPECT_EQ(static_cast<uint64_t>(arr[0]), 0x1000u);
    EXPECT_EQ(static_cast<uint64_t>(arr[1]), 0x2000u);
    EXPECT_EQ(static_cast<uint64_t>(arr[2]), 0x3000u);
}

TEST(EditorTest, ArrayRoundtrip) {
    elf_editor<elf64_traits> ed;
    ed.create(ELFDATA2LSB, ET_EXEC, EM_X86_64);

    array_builder<uint64_t> ab;
    ab.add(0x400100);
    ab.add(0x400200);

    endian_convertor conv(byte_order::little);
    auto arr_data = ab.build(conv);

    auto& sec = ed.add_section(".init_array", SHT_INIT_ARRAY, SHF_ALLOC | SHF_WRITE);
    sec.set_data(arr_data.data(), arr_data.size());
    sec.set_entry_size(sizeof(uint64_t));
    sec.set_addr_align(8);

    auto result = ed.save();
    ASSERT_TRUE(result.has_value());

    byte_view view{result->data(), result->size()};
    auto file = elf_file<elf64_traits>::from_view(view);
    ASSERT_TRUE(file.has_value());

    auto init_sec = file->find_section(".init_array");
    ASSERT_TRUE(init_sec.has_value());
    EXPECT_EQ(init_sec->type(), SHT_INIT_ARRAY);

    array_range<uint64_t> arr{init_sec->data(), &file->convertor()};
    EXPECT_EQ(arr.size(), 2u);
    EXPECT_EQ(static_cast<uint64_t>(arr[0]), 0x400100u);
}

// ================================================================
//  Modinfo builder tests
// ================================================================

TEST(EditorTest, ModinfoBuilder) {
    modinfo_builder mb;
    mb.add("license", "GPL");
    mb.add("author", "test");
    EXPECT_EQ(mb.count(), 2u);

    auto data = mb.build();
    EXPECT_GT(data.size(), 0u);

    // Parse back
    byte_view bv{data.data(), data.size()};
    modinfo_range mi{bv};
    std::size_t count = 0;
    for (auto attr : mi) {
        if (count == 0) {
            EXPECT_EQ(attr.key(), "license");
            EXPECT_EQ(attr.value(), "GPL");
        }
        if (count == 1) {
            EXPECT_EQ(attr.key(), "author");
            EXPECT_EQ(attr.value(), "test");
        }
        ++count;
    }
    EXPECT_EQ(count, 2u);
}

// ================================================================
//  Versym builder tests
// ================================================================

TEST(EditorTest, VersymBuilder) {
    versym_builder vb;
    vb.add(0); // VER_NDX_LOCAL
    vb.add(1); // VER_NDX_GLOBAL
    vb.add(2); // some defined version
    EXPECT_EQ(vb.count(), 3u);

    endian_convertor conv(byte_order::little);
    auto data = vb.build(conv);
    EXPECT_EQ(data.size(), 3u * sizeof(Elf_Half));

    // Parse back
    byte_view bv{data.data(), data.size()};
    versym_range vs{bv, &conv};
    EXPECT_EQ(vs.size(), 3u);

    std::vector<Elf_Half> values;
    for (auto v : vs) values.push_back(v.value());
    EXPECT_EQ(values.size(), 3u);
    EXPECT_EQ(values[0], 0u);
    EXPECT_EQ(values[1], 1u);
    EXPECT_EQ(values[2], 2u);
}
