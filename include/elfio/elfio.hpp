#ifndef ELFIO_MODERN_ELFIO_HPP
#define ELFIO_MODERN_ELFIO_HPP

// ── core layer ──────────────────────────────────────────────────
#include "core/error.hpp"
#include "core/result.hpp"
#include "core/safe_math.hpp"
#include "core/byte_view.hpp"
#include "core/endian.hpp"
#include "core/lazy.hpp"
#include "core/hash.hpp"

// ── platform layer ──────────────────────────────────────────────
#include "platform/schema.hpp"
#include "platform/traits.hpp"

// ── views layer (read-only, zero-copy) ──────────────────────────
#include "views/string_table.hpp"
#include "views/section_view.hpp"
#include "views/segment_view.hpp"
#include "views/symbol_view.hpp"
#include "views/relocation_view.hpp"
#include "views/dynamic_view.hpp"
#include "views/note_view.hpp"
#include "views/versym_view.hpp"
#include "views/modinfo_view.hpp"
#include "views/array_view.hpp"
#include "views/dump.hpp"
#include "views/elf_file.hpp"

// ── editor layer (mutable, owned data) ──────────────────────────
#include "editor/string_table_builder.hpp"
#include "editor/section_entry.hpp"
#include "editor/segment_entry.hpp"
#include "editor/symbol_builder.hpp"
#include "editor/relocation_builder.hpp"
#include "editor/dynamic_builder.hpp"
#include "editor/note_builder.hpp"
#include "editor/array_builder.hpp"
#include "editor/modinfo_builder.hpp"
#include "editor/versym_builder.hpp"
#include "editor/layout.hpp"
#include "editor/elf_editor.hpp"

#endif // ELFIO_MODERN_ELFIO_HPP
