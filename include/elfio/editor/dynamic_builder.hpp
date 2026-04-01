#pragma once

/// Mutable dynamic section builder for writing ELF files.

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <elfio/core/endian.hpp>
#include <elfio/platform/schema.hpp>
#include <elfio/platform/traits.hpp>
#include <elfio/editor/string_table_builder.hpp>

namespace elfio {

template <typename Traits>
class dynamic_builder {
public:
    struct dyn_entry {
        int64_t  tag   = 0;
        uint64_t value = 0;
        std::string str_value;  // for DT_NEEDED etc., resolved during build
    };

    void add(int64_t tag, uint64_t value) {
        entries_.push_back({tag, value, {}});
    }

    void add_needed(std::string library_name) {
        entries_.push_back({DT_NEEDED, 0, std::move(library_name)});
    }

    void add_soname(std::string name) {
        entries_.push_back({DT_SONAME, 0, std::move(name)});
    }

    void add_rpath(std::string path) {
        entries_.push_back({DT_RPATH, 0, std::move(path)});
    }

    void add_runpath(std::string path) {
        entries_.push_back({DT_RUNPATH, 0, std::move(path)});
    }

    [[nodiscard]] std::size_t count() const noexcept { return entries_.size(); }
    [[nodiscard]] const std::vector<dyn_entry>& entries() const noexcept { return entries_; }

    /// Build the binary dynamic section data. String entries are resolved
    /// through the provided string table builder.
    [[nodiscard]] std::vector<char> build(string_table_builder& strtab,
                                          const endian_convertor& conv) const {
        using dyn_t = typename Traits::dyn_type;
        // +1 for DT_NULL terminator
        std::size_t total = entries_.size() + 1;
        std::vector<char> out(total * sizeof(dyn_t), '\0');

        for (std::size_t i = 0; i < entries_.size(); ++i) {
            const auto& e = entries_[i];
            dyn_t d{};

            uint64_t val = e.value;
            if (!e.str_value.empty()) {
                val = strtab.add(e.str_value);
            }

            if constexpr (std::is_same_v<Traits, elf32_traits>) {
                d.d_tag    = conv(static_cast<Elf_Sword>(e.tag));
                d.d_un.d_val = conv(static_cast<Elf_Word>(val));
            } else {
                d.d_tag    = conv(static_cast<Elf_Sxword>(e.tag));
                d.d_un.d_val = conv(static_cast<Elf_Xword>(val));
            }

            std::memcpy(out.data() + i * sizeof(dyn_t), &d, sizeof(dyn_t));
        }

        // DT_NULL terminator (already zeroed by vector init)
        return out;
    }

    void clear() { entries_.clear(); }

private:
    std::vector<dyn_entry> entries_;
};

} // namespace elfio
