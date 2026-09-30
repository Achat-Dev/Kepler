// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2026 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "ast/ast_node.hpp"
#include "semantic_analysis/scope.hpp"
#include "type_system/type.hpp"
#include "utils/assert.h"
#include "utils/string_pool.hpp"
#include <cstddef>
#include <cstdint>
#include <format>
#include <functional>
#include <limits>
#include <variant>
#include <vector>

namespace kepler {

    struct SymbolId {
        uint32_t value = std::numeric_limits<uint32_t>::max();

        bool operator==(const SymbolId& other) const = default;
        bool operator!=(const SymbolId& other) const = default;
        static constexpr SymbolId invalid() { return SymbolId{}; }
    };

    enum class SymbolKind {
        All,
        Variable,
        Prototype,
        Type,
    };

    struct PrototypeSymbolData {
        LinkageType linkage_type;
        bool is_variadic = false;
        std::vector<TypeId> parameter_type_ids;
    };

    using SymbolData = std::variant<std::monostate, PrototypeSymbolData>;

    struct Symbol {
        SymbolId id;
        ScopeId scope_id;
        TypeId type_id;
        StringId identifier_id;
        StringId mangled_identifier_id;
        SymbolKind symbol_kind;
        SymbolData data = std::monostate{};
    };

}

template <>
struct std::hash<kepler::SymbolId> {
    size_t operator()(const kepler::SymbolId& id) const noexcept {
        return hash<uint32_t>{}(id.value);
    }
};

template <>
struct std::formatter<kepler::SymbolKind> : std::formatter<std::string> {
    auto format(const kepler::SymbolKind& symbol_kind, std::format_context& ctx) const {
        switch (symbol_kind) {
            case kepler::SymbolKind::All:
                return std::formatter<std::string>::format("all", ctx);
            case kepler::SymbolKind::Variable:
                return std::formatter<std::string>::format("variable", ctx);
            case kepler::SymbolKind::Prototype:
                return std::formatter<std::string>::format("prototype", ctx);
            case kepler::SymbolKind::Type:
                return std::formatter<std::string>::format("type", ctx);
        }
        KPL_ASSERT_UNREACHABLE("Missing format implementation for symbol kind '{}'", static_cast<int>(symbol_kind));
    }
};
