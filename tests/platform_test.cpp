#include <gtest/gtest.h>
#include <elfio/platform/schema.hpp>
#include <elfio/platform/traits.hpp>

using namespace elfio;

// ================================================================
//  Struct size verification
// ================================================================

TEST(SchemaTest, Elf32EhdrSize) { EXPECT_EQ(sizeof(Elf32_Ehdr), 52u); }
TEST(SchemaTest, Elf64EhdrSize) { EXPECT_EQ(sizeof(Elf64_Ehdr), 64u); }
TEST(SchemaTest, Elf32ShdrSize) { EXPECT_EQ(sizeof(Elf32_Shdr), 40u); }
TEST(SchemaTest, Elf64ShdrSize) { EXPECT_EQ(sizeof(Elf64_Shdr), 64u); }
TEST(SchemaTest, Elf32PhdrSize) { EXPECT_EQ(sizeof(Elf32_Phdr), 32u); }
TEST(SchemaTest, Elf64PhdrSize) { EXPECT_EQ(sizeof(Elf64_Phdr), 56u); }
TEST(SchemaTest, Elf32SymSize)  { EXPECT_EQ(sizeof(Elf32_Sym),  16u); }
TEST(SchemaTest, Elf64SymSize)  { EXPECT_EQ(sizeof(Elf64_Sym),  24u); }
TEST(SchemaTest, Elf32RelSize)  { EXPECT_EQ(sizeof(Elf32_Rel),  8u);  }
TEST(SchemaTest, Elf64RelSize)  { EXPECT_EQ(sizeof(Elf64_Rel),  16u); }
TEST(SchemaTest, Elf32RelaSize) { EXPECT_EQ(sizeof(Elf32_Rela), 12u); }
TEST(SchemaTest, Elf64RelaSize) { EXPECT_EQ(sizeof(Elf64_Rela), 24u); }
TEST(SchemaTest, Elf32DynSize)  { EXPECT_EQ(sizeof(Elf32_Dyn),  8u);  }
TEST(SchemaTest, Elf64DynSize)  { EXPECT_EQ(sizeof(Elf64_Dyn),  16u); }
TEST(SchemaTest, ElfNhdrSize)   { EXPECT_EQ(sizeof(Elf_Nhdr),   12u); }

// ================================================================
//  Symbol info helpers
// ================================================================

TEST(SchemaTest, SymbolInfoHelpers) {
    auto info = elf_st_info(STB_GLOBAL, STT_FUNC);
    EXPECT_EQ(elf_st_bind(info), STB_GLOBAL);
    EXPECT_EQ(elf_st_type(info), STT_FUNC);
}

// ================================================================
//  Relocation info helpers
// ================================================================

TEST(SchemaTest, Elf32RelocationHelpers) {
    auto info = elf32_r_info(10, R_386_32);
    EXPECT_EQ(elf32_r_sym(info), 10u);
    EXPECT_EQ(elf32_r_type(info), R_386_32);
}

TEST(SchemaTest, Elf64RelocationHelpers) {
    auto info = elf64_r_info(42, R_X86_64_64);
    EXPECT_EQ(elf64_r_sym(info), 42u);
    EXPECT_EQ(elf64_r_type(info), static_cast<uint64_t>(R_X86_64_64));
}

// ================================================================
//  Traits
// ================================================================

TEST(TraitsTest, Elf32Traits) {
    EXPECT_EQ(elf32_traits::file_class, ELFCLASS32);
    EXPECT_EQ(elf32_traits::addr_size, 4u);
    EXPECT_EQ(sizeof(elf32_traits::ehdr_type), 52u);
}

TEST(TraitsTest, Elf64Traits) {
    EXPECT_EQ(elf64_traits::file_class, ELFCLASS64);
    EXPECT_EQ(elf64_traits::addr_size, 8u);
    EXPECT_EQ(sizeof(elf64_traits::ehdr_type), 64u);
}

// ================================================================
//  Constants sanity
// ================================================================

TEST(SchemaTest, ElfMagic) {
    EXPECT_EQ(ELFMAG0, 0x7F);
    EXPECT_EQ(ELFMAG1, 'E');
    EXPECT_EQ(ELFMAG2, 'L');
    EXPECT_EQ(ELFMAG3, 'F');
}

TEST(SchemaTest, SectionTypes) {
    EXPECT_EQ(SHT_NULL,     0u);
    EXPECT_EQ(SHT_PROGBITS, 1u);
    EXPECT_EQ(SHT_SYMTAB,   2u);
    EXPECT_EQ(SHT_STRTAB,   3u);
    EXPECT_EQ(SHT_RELA,     4u);
    EXPECT_EQ(SHT_DYNAMIC,  6u);
    EXPECT_EQ(SHT_NOTE,     7u);
    EXPECT_EQ(SHT_NOBITS,   8u);
    EXPECT_EQ(SHT_REL,      9u);
    EXPECT_EQ(SHT_DYNSYM,   11u);
}

TEST(SchemaTest, SegmentTypes) {
    EXPECT_EQ(PT_NULL,    0u);
    EXPECT_EQ(PT_LOAD,    1u);
    EXPECT_EQ(PT_DYNAMIC, 2u);
    EXPECT_EQ(PT_INTERP,  3u);
    EXPECT_EQ(PT_NOTE,    4u);
    EXPECT_EQ(PT_PHDR,    6u);
}
