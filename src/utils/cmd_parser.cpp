// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2026 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "utils/cmd_parser.hpp"
#include "diagnostics/diagnostic.hpp"
#include "utils/ansi_codes.hpp"
#include "utils/assert.h"
#include "utils/log.hpp"
#include "utils/string_pool.hpp"
#include "utils/string_utils.hpp"
#include "version.hpp"
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <expected>
#include <format>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace kepler {

    std::expected<void, Diagnostic> CmdParser::parse(int argc, char** argv) {
        if (argc == 1) {
            // No option given, just set help option to true if it exists
            const StringId help_id = StringPool::get().store("help");
            CmdOption* help_option = find_option(help_id);
            if (help_option != nullptr) {
                return help_option->set_value({"true"}, help_option, 0);
            }
            return {};
        }

        // Save original values for diagnostics
        for (int i = 0; i < argc; i++) {
            original_args.push_back(std::string(argv[i]));
        }

        // Parse args
        int index = 1;
        while (index < argc) {
            const std::string arg(argv[index]);
            CmdOption* option;
            if (arg.starts_with("--")) {
                const std::string long_arg_name = arg.substr(2);
                StringId long_arg_name_id = StringPool::get().store(long_arg_name);
                option = find_option(long_arg_name_id);
                if (option == nullptr) {
                    return std::unexpected(Diagnostic{
                        .code = DiagnosticCode::UnknownOption,
                        .message = std::format("Unknown cmd option '{}'\n{}", arg, get_arg_string_with_highlighted_error({index})),
                    });
                }
            } else if (arg.starts_with('-')) {
                const std::string short_arg_name_string = arg.substr(1);
                if (short_arg_name_string.size() != 1) {
                    return std::unexpected(Diagnostic{
                        .code = DiagnosticCode::InvalidOptionFormat,
                        .message = std::format("Unknown cmd option '{}'\n{}", arg, get_arg_string_with_highlighted_error({index})),
                    });
                }

                char short_arg_name = short_arg_name_string[0];
                if (!std::isalpha(short_arg_name)) {
                    return std::unexpected(Diagnostic{
                        .code = DiagnosticCode::InvalidOptionFormat,
                        .message = std::format("Short version of cmd option must be a alphabetical character, received '{}'\n{}",
                            short_arg_name,
                            get_arg_string_with_highlighted_error({index})),
                    });
                }

                option = find_option(short_arg_name);
                if (option == nullptr) {
                    return std::unexpected(Diagnostic{
                        .code = DiagnosticCode::UnknownOption,
                        .message = std::format("Unknown cmd option '{}'\n{}", arg, get_arg_string_with_highlighted_error({index})),
                    });
                }
            }

            KPL_ASSERT_NOT_NULLPTR(option);
            if (option->original_arg_index != -1) {
                return std::unexpected(Diagnostic{
                    .code = DiagnosticCode::OptionUsedTooOften,
                    .message = std::format("Cmd option '-{}/--{}' can only be used once\n{}",
                        option->short_name,
                        StringPool::get().lookup(option->long_name_id),
                        get_arg_string_with_highlighted_error({option->original_arg_index, index})),
                });
            }
            option->original_arg_index = index;

            switch (option->value_type) {
                case CmdOptionValueType::NoValue: {
                    const auto set_result = option->set_value({"true"}, option, index);
                    if (!set_result) {
                        return std::unexpected(set_result.error());
                    }
                    if (index < argc - 1) {
                        if (argv[index + 1][0] != '-') {
                            return std::unexpected(Diagnostic{
                                .code = DiagnosticCode::InvalidOptionValue,
                                .message = std::format("Cmd option '-{}/--{}' doesn't take a value\n{}",
                                    option->short_name,
                                    StringPool::get().lookup(option->long_name_id),
                                    get_arg_string_with_highlighted_error({index})),
                            });
                        }
                    }
                    break;
                }
                case CmdOptionValueType::OneValue: {
                    if (index >= argc - 1) {
                        return std::unexpected(Diagnostic{
                            .code = DiagnosticCode::MissingOptionValue,
                            .message = std::format("Cmd option '-{}'/'--{}' needs a value\n{}",
                                option->short_name,
                                StringPool::get().lookup(option->long_name_id),
                                get_arg_string_with_highlighted_error({index})),
                        });
                    }

                    index += 1;
                    if (argv[index][0] == '-') {
                        return std::unexpected(Diagnostic{
                            .code = DiagnosticCode::MissingOptionValue,
                            .message = std::format("Cmd option '-{}'/'--{}' needs a value\n{}",
                                option->short_name,
                                StringPool::get().lookup(option->long_name_id),
                                get_arg_string_with_highlighted_error({index})),
                        });
                    }
                    const auto set_result = option->set_value({std::string(argv[index])}, option, index);
                    if (!set_result) {
                        return std::unexpected(set_result.error());
                    }

                    if (index + 1 <= argc - 1) {
                        if (argv[index + 1][0] != '-') {
                            std::vector<int> faulty_arg_indices{option->original_arg_index, index};
                            index += 1;
                            while (index < argc) {
                                if (argv[index][0] == '-') {
                                    break;
                                }
                                faulty_arg_indices.push_back(index);
                                index += 1;
                            }
                            const int faulty_arg_count = faulty_arg_indices.size() - 1; // -1 because the arg option is also part of the vector
                            return std::unexpected(Diagnostic{
                                .code = DiagnosticCode::InvalidOptionValue,
                                .message = std::format("Cmd option '-{}/--{}' only takes one value, received {} values\n{}",
                                    option->short_name,
                                    StringPool::get().lookup(option->long_name_id),
                                    faulty_arg_count,
                                    get_arg_string_with_highlighted_error(std::move(faulty_arg_indices))),
                            });
                        }
                    }
                    break;
                }
                case CmdOptionValueType::List: {
                    if (index >= argc - 1) {
                        return std::unexpected(Diagnostic{
                            .code = DiagnosticCode::MissingOptionValue,
                            .message = std::format("Cmd option '-{}'/'--{}' needs a value\n{}",
                                option->short_name,
                                StringPool::get().lookup(option->long_name_id),
                                get_arg_string_with_highlighted_error({index})),
                        });
                    }

                    index += 1;
                    if (argv[index][0] == '-') {
                        return std::unexpected(Diagnostic{
                            .code = DiagnosticCode::MissingOptionValue,
                            .message = std::format("Cmd option '-{}'/'--{}' needs a value\n{}",
                                option->short_name,
                                StringPool::get().lookup(option->long_name_id),
                                get_arg_string_with_highlighted_error({index})),
                        });
                    }

                    std::vector<std::string> args;
                    while (index < argc) {
                        if (argv[index][0] == '-') {
                            break;
                        }
                        args.push_back(std::string(argv[index]));
                        index += 1;
                    }
                    const auto set_result = option->set_value({args}, option, index);
                    if (!set_result) {
                        return std::unexpected(set_result.error());
                    }
                    index -= 1; // Go back one because otherwise one argument will be skipped
                    break;
                }
            }

            index += 1;
        }

        return {};
    }

    std::string CmdParser::get_help() const {
        std::string result = std::format("{} (version {}.{}.{})\n",
            StringPool::get().lookup(program_description_id),
            version.major,
            version.minor,
            version.patch);
        result += std::format("Usage:\n    kepler [options...]\n\nAvailable options:\n");
        for (const CmdOption& option : options) {
            result += std::format("    -{},--{}", option.short_name, StringPool::get().lookup(option.long_name_id));
            switch (option.value_type) {
                case CmdOptionValueType::NoValue:
                    result += '\n';
                    break;
                case CmdOptionValueType::OneValue:
                    result += " <arg>\n";
                    break;
                case CmdOptionValueType::List:
                    result += " <args...>\n";
                    break;
            }
            result += get_indent_wrapped_text(std::string(StringPool::get().lookup(option.description_id)), wrapped_text_width, "        ");
            if (!result.ends_with('\n')) {
                result += '\n';
            }
        }

        // Remove trailing whitespaces and newlines
        const auto pos = result.find_last_not_of(" \t\n");
        if (pos != std::string::npos) {
            result.erase(pos + 1);
        }
        return result;
    }

    CmdOption* CmdParser::find_option(char short_name) {
        for (CmdOption& option : options) {
            if (option.short_name == short_name) {
                return &option;
            }
        }
        return nullptr;
    }

    CmdOption* CmdParser::find_option(StringId long_name_id) {
        for (CmdOption& option : options) {
            if (option.long_name_id == long_name_id) {
                return &option;
            }
        }
        return nullptr;
    }

    std::string CmdParser::get_arg_string_with_highlighted_error(std::vector<int> indices_to_highlight) {
        const std::string prefix = std::string(log::last_indented) + "Used command: ";
        std::string command = prefix + original_args[0];
        std::string arrows = std::string(strlen_utf8(prefix) + strlen_utf8(original_args[0]), ' ');
        int last_arrow_index = -1;
        for (int i = 1; i < original_args.size(); i++) {
            if (std::find(indices_to_highlight.begin(), indices_to_highlight.end(), i) == indices_to_highlight.end()) {
                command += ' ' + original_args[i];
                arrows += std::string(strlen_utf8(original_args[i]) + 1, ' '); // +1 for the space
            } else {
                const std::string severity_highlight = get_severity_highlight(DiagnosticSeverity::Error);
                command += std::format(" {}{}{}", severity_highlight, original_args[i], ansi_codes::reset);
                if (last_arrow_index == i - 1) {
                    // Both the last and this arg are errors, so also highlight the space between
                    arrows += std::format("{}{}{}", severity_highlight, std::string(strlen_utf8(original_args[i]) + 1, '^'), ansi_codes::reset);
                } else {
                    arrows += std::format(" {}{}{}", severity_highlight, std::string(strlen_utf8(original_args[i]), '^'), ansi_codes::reset);
                }
                last_arrow_index = i;
            }
        }
        return command + '\n' + arrows;
    }

    std::string CmdParser::get_indent_wrapped_text(const std::string& text, uint32_t width, const std::string& indent) const {
        std::string result;
        const uint32_t indent_size = indent.size();
        uint32_t current_width = 0;
        std::istringstream stream(text);
        std::string line;
        while (std::getline(stream, line)) {
            result += indent;
            current_width += indent_size;
            if (current_width + line.size() <= width) {
                result += line + '\n';
                current_width = 0;
            } else {
                std::istringstream words(line);
                std::string word;
                while (std::getline(words, word, ' ')) {
                    const uint32_t word_size = word.size() + 1; // +1 for the extra space after the word
                    if (current_width + word_size <= width) {
                        result += word + ' ';
                        current_width += word_size;
                    } else {
                        result += '\n' + indent + word + ' ';
                        current_width = indent_size + word_size;
                    }
                }
                result += '\n';
                current_width = 0;
            }
        }
        return result;
    }

}
