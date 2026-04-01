#pragma once

/// Mutable relocation table builder for writing ELF files.
/// Builds both REL and RELA section data.

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>
#include <elfio/core/endian.hpp>
#include <elfio/platform/schema.hpp>
#include <elfio/platform/traits.hpp>

namespace elfio {

template <typename Traits>
class relocation_builder {
public:
    struct rel_entry {
        uint64_t offset = 0;
        uint64_t sym    = 0;
        uint64_t type   = 0;
    };

    struct rela_entry {
        uint64_t offset = 0;
        uint64_t sym    = 0;
        uint64_t type   = 0;
        int64_t  addend = 0;
    };

    void add_rel(uint64_t offset, uint64_t sym, uint64_t type) {
        rels_.push_back({offset, sym, type});
    }

    void add_rela(uint64_t offset, uint64_t sym, uint64_t type, int64_t addend) {
        relas_.push_back({offset, sym, type, addend});
    }

    [[nodiscard]] std::size_t rel_count()  const noexcept { return rels_.size(); }
    [[nodiscard]] std::size_t rela_count() const noexcept { return relas_.size(); }

    [[nodiscard]] std::vector<char> build_rel(const endian_convertor& conv) const {
        using rel_t = typename Traits::rel_type;
        std::vector<char> out(rels_.size() * sizeof(rel_t), '\0');

        for (std::size_t i = 0; i < rels_.size(); ++i) {
            const auto& e = rels_[i];
            rel_t r{};
            auto info = Traits::r_info(e.sym, e.type);

            if constexpr (std::is_same_v<Traits, elf32_traits>) {
                r.r_offset = conv(static_cast<Elf32_Addr>(e.offset));
                r.r_info   = conv(static_cast<Elf_Word>(info));
            } else {
                r.r_offset = conv(static_cast<Elf64_Addr>(e.offset));
                r.r_info   = conv(static_cast<Elf_Xword>(info));
            }

            std::memcpy(out.data() + i * sizeof(rel_t), &r, sizeof(rel_t));
        }
        return out;
    }

    [[nodiscard]] std::vector<char> build_rela(const endian_convertor& conv) const {
        using rela_t = typename Traits::rela_type;
        std::vector<char> out(relas_.size() * sizeof(rela_t), '\0');

        for (std::size_t i = 0; i < relas_.size(); ++i) {
            const auto& e = relas_[i];
            rela_t r{};
            auto info = Traits::r_info(e.sym, e.type);

            if constexpr (std::is_same_v<Traits, elf32_traits>) {
                r.r_offset = conv(static_cast<Elf32_Addr>(e.offset));
                r.r_info   = conv(static_cast<Elf_Word>(info));
                r.r_addend = conv(static_cast<Elf_Sword>(e.addend));
            } else {
                r.r_offset = conv(static_cast<Elf64_Addr>(e.offset));
                r.r_info   = conv(static_cast<Elf_Xword>(info));
                r.r_addend = conv(static_cast<Elf_Sxword>(e.addend));
            }

            std::memcpy(out.data() + i * sizeof(rela_t), &r, sizeof(rela_t));
        }
        return out;
    }

    void clear() { rels_.clear(); relas_.clear(); }

private:
    std::vector<rel_entry>  rels_;
    std::vector<rela_entry> relas_;
};

} // namespace elfio
