// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2026 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "diagnostics/diagnostic.hpp"
#include "utils/assert.h"
#include "utils/string_pool.hpp"
#include <charconv>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <functional>
#include <string>
#include <system_error>
#include <type_traits>
#include <utility>
#include <vector>

namespace kepler {

    enum class CmdOptionValueType {
        NoValue,
        OneValue,
        List,
    };

    struct CmdOption {
        bool option_used = false;
        char short_name = ' ';
        StringId long_name_id;
        StringId description_id;
        CmdOptionValueType value_type;

        std::function<std::expected<void, Diagnostic>(const std::vector<std::string>&, int original_arg_index)> set_value;
    };

    template <typename T>
    struct is_vector : std::false_type {};

    template <typename T, typename Alloc>
    struct is_vector<std::vector<T, Alloc>> : std::true_type {
        using value_type = T;
    };

    class CmdParser {
    public:
        CmdParser(std::string program_description)
            : program_description_id(StringPool::get().store(std::move(program_description))) {}

        std::expected<void, Diagnostic> parse(int argc, char** argv);
        std::string get_help() const;

        template <typename T>
        void add_option(T* value, char short_name, std::string long_name, std::string description) {
            KPL_ASSERT_NOT_NULLPTR(value);
            KPL_ASSERT_THAT(!find_option(short_name), "Option with short name '{}' is already registered", short_name);
            const StringId long_name_id = StringPool::get().store(long_name);
            KPL_ASSERT_THAT(!find_option(long_name_id), "Option with long name '{}' is already registered", long_name);
            CmdOptionValueType value_type = CmdOptionValueType::OneValue;
            if constexpr (std::is_same_v<T, bool>) {
                value_type = CmdOptionValueType::NoValue;
            } else if constexpr (kepler::is_vector<std::remove_cvref_t<T>>::value) {
                using vector_type = typename kepler::is_vector<std::remove_cvref_t<T>>::value_type;
                if constexpr (kepler::is_vector<std::remove_cvref_t<vector_type>>::value) {
                    KPL_ASSERT_THAT(true, "Cmd option '{}' is a nested vector, which is not supported", long_name);
                }
                value_type = CmdOptionValueType::List;
            }

            options.push_back(CmdOption{
                .option_used = false,
                .short_name = short_name,
                .long_name_id = long_name_id,
                .description_id = StringPool::get().store(std::move(description)),
                .value_type = value_type,
                .set_value = [this, value](const std::vector<std::string>& args, int original_arg_index) {
                    return set_value(*value, args, original_arg_index);
                }});
        }

    private:
        const StringId program_description_id;
        std::vector<std::string> original_args;
        std::vector<CmdOption> options;

        CmdOption* find_option(char short_name);
        CmdOption* find_option(StringId long_name_id);
        std::string get_args_with_highlighted_error(int index_to_highlight);
        std::string get_indent_wrapped_text(const std::string& text, uint32_t width, const std::string& indent) const;

        template <typename T>
            requires std::is_integral_v<T> || std::is_floating_point_v<T>
        bool try_parse(const std::string& text, T& value) {
            const auto [ptr, error_code] = std::from_chars(text.data(), text.data() + text.size(), value);
            return error_code == std::errc{} && ptr == text.data() + text.size();
        }

        template <typename T>
        std::expected<void, Diagnostic> set_value(T& value, const std::vector<std::string>& args, int original_arg_index) {
            if constexpr (std::is_same_v<T, bool>) {
                KPL_ASSERT_THAT(args.size() == 1, "Setting boolean cmd option requires one argument");
                KPL_ASSERT_THAT(args[0] == "true", "Setting boolean cmd option requires argument to be true");
                value = true;
            } else if constexpr (is_vector<T>::value) {
                KPL_ASSERT_THAT(args.size() > 0, "Setting vector cmd option requires at least argument");
                using TValueType = typename is_vector<T>::value_type;
                for (const auto& arg : args) {
                    const auto result = parse_value<TValueType>(arg, original_arg_index);
                    if (!result) {
                        return std::unexpected(result.error());
                    }
                    value.push_back(*result);
                }
            } else {
                KPL_ASSERT_THAT(args.size() == 1, "Setting cmd option requires exactly one argument");
                const auto result = parse_value<T>(args[0], original_arg_index);
                if (!result) {
                    return std::unexpected(result.error());
                }
                value = *result;
            }
            return {};
        }

        template <typename T>
        std::expected<T, Diagnostic> parse_value(const std::string& text, int original_arg_index) {
            if constexpr (std::is_same_v<T, std::string>) {
                return text;
            } else if constexpr (std::is_same_v<T, std::filesystem::path>) {
                return std::filesystem::path(text);
            } else if constexpr (std::is_integral_v<T> || std::is_floating_point_v<T>) {
                T result{};
                if (!try_parse(text, result)) {
                    return std::unexpected(Diagnostic{
                        .code = DiagnosticCode::InvalidOptionValue,
                        .message = std::format("Cmd option value '{}' is not an integer\n{}", text, get_args_with_highlighted_error(original_arg_index)),
                    });
                }
                return result;
            }
            KPL_ASSERT_UNREACHABLE("Missing parse arg implementation for type '{}'", typeid(T).name());
        }
    };

}
