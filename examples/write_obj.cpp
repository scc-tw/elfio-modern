/// Example: write a minimal ELF relocatable object file using elfio-modern editor.

#include <elfio/elfio.hpp>

#include <cstdio>
#include <fstream>

int main() {
    using namespace elfio;

    elf_editor<elf64_traits> ed;
    ed.create(ELFDATA2LSB, ET_REL, EM_X86_64);
    ed.set_os_abi(ELFOSABI_LINUX);

    // .text section with a simple "ret" instruction
    unsigned char code[] = { 0xc3 };  // x86_64 RET
    auto& text = ed.add_section(".text", SHT_PROGBITS,
                                 SHF_ALLOC | SHF_EXECINSTR);
    text.set_data(reinterpret_cast<const char*>(code), sizeof(code));
    text.set_addr_align(16);

    // .data section
    const char data[] = "Hello, elfio-modern!";
    auto& data_sec = ed.add_section(".data", SHT_PROGBITS,
                                     SHF_ALLOC | SHF_WRITE);
    data_sec.set_data(data, sizeof(data));
    data_sec.set_addr_align(4);

    // Build symbol table
    string_table_builder symstrtab;
    symbol_builder<elf64_traits> syms;

    // Add _start symbol
    (void)syms.add("_start", 0, 0, STB_GLOBAL, STT_FUNC, STV_DEFAULT,
                    2);  // section index 2 = .text

    // Add message symbol
    (void)syms.add("message", 0, sizeof(data), STB_GLOBAL, STT_OBJECT,
                    STV_DEFAULT, 3);  // section index 3 = .data

    // We need a convertor for the build
    endian_convertor conv(byte_order::little);

    // Build the symbol table binary data
    auto symdata = syms.build(symstrtab, conv);

    // Add .strtab section for symbol names
    auto& strtab_sec = ed.add_section(".strtab", SHT_STRTAB);
    strtab_sec.set_data(symstrtab.data());

    // Add .symtab section
    auto& symtab_sec = ed.add_section(".symtab", SHT_SYMTAB);
    symtab_sec.set_data(symdata.data(), symdata.size());
    symtab_sec.set_entry_size(sizeof(Elf64_Sym));
    symtab_sec.set_addr_align(8);

    // .symtab's sh_link = .strtab index, sh_info = first non-local symbol
    // Sections: 0=null, 1=.text, 2=.data, 3=.strtab, 4=.symtab, 5=.shstrtab
    symtab_sec.set_link(3);  // index of .strtab
    symtab_sec.set_info(1);  // first global symbol index

    // Save
    auto result = ed.save();
    if (!result) {
        std::fprintf(stderr, "Failed to save: %s\n",
                     std::string(to_string(result.error())).c_str());
        return 1;
    }

    // Write to file
    std::ofstream out("output.o", std::ios::binary);
    out.write(result->data(), static_cast<std::streamsize>(result->size()));
    std::printf("Wrote output.o (%zu bytes)\n", result->size());

    return 0;
}
