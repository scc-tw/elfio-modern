#pragma once

/// Mutable symbol table builder for writing ELF files.
/// Accumulates symbol entries and produces the binary symbol table section data.

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <elfio/core/endian.hpp>
#include <elfio/platform/schema.hpp>
#include <elfio/editor/string_table_builder.hpp>

namespace elfio {

struct symbol_entry_data {
    std::string   name;
    uint64_t      value      = 0;
    uint64_t      size       = 0;
    unsigned char bind       = STB_LOCAL;
    unsigned char type       = STT_NOTYPE;
    unsigned char other      = STV_DEFAULT;
    Elf_Half      shndx      = SHN_UNDEF;
};

template <typename Traits>
class symbol_builder {
    std::vector<symbol_entry_data> entries_;

public:
    symbol_builder() {
        // ELF symbol tables must start with an undefined symbol (index 0)
        entries_.emplace_back();
    }

    /// Add a symbol and return its index.
    [[nodiscard]] std::size_t add(symbol_entry_data sym) {
        auto idx = entries_.size();
        entries_.push_back(std::move(sym));
        return idx;
    }

    /// Add a symbol by fields and return its index.
    [[nodiscard]] std::size_t add(std::string name, uint64_t value, uint64_t size,
                                  unsigned char bind, unsigned char type,
                                  unsigned char other, Elf_Half shndx) {
        symbol_entry_data sym;
        sym.name  = std::move(name);
        sym.value = value;
        sym.size  = size;
        sym.bind  = bind;
        sym.type  = type;
        sym.other = other;
        sym.shndx = shndx;
        return add(std::move(sym));
    }

    [[nodiscard]] std::size_t count() const noexcept { return entries_.size(); }
    [[nodiscard]] const std::vector<symbol_entry_data>& entries() const noexcept { return entries_; }

    /// Build the binary symbol table data and populate the string table.
    [[nodiscard]] std::vector<char> build(string_table_builder& strtab,
                                          const endian_convertor& conv) const {
        using sym_t = typename Traits::sym_type;
        std::vector<char> result(entries_.size() * sizeof(sym_t), '\0');

        for (std::size_t i = 0; i < entries_.size(); ++i) {
            const auto& e = entries_[i];
            sym_t sym{};

            Elf_Word name_idx = e.name.empty() ? 0 : strtab.add(e.name);
            sym.st_name  = conv(name_idx);
            sym.st_info  = elf_st_info(e.bind, e.type);
            sym.st_other = e.other;
            sym.st_shndx = conv(e.shndx);

            // Handle value/size based on traits (field order differs for 32/64)
            if constexpr (std::is_same_v<Traits, elf32_traits>) {
                sym.st_value = conv(static_cast<Elf32_Addr>(e.value));
                sym.st_size  = conv(static_cast<Elf_Word>(e.size));
            } else {
                sym.st_value = conv(static_cast<Elf64_Addr>(e.value));
                sym.st_size  = conv(static_cast<Elf_Xword>(e.size));
            }

            std::memcpy(result.data() + i * sizeof(sym_t), &sym, sizeof(sym_t));
        }

        return result;
    }

    void clear() {
        entries_.clear();
        entries_.emplace_back(); // re-add undefined symbol at index 0
    }
};

} // namespace elfio
