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
#include "utils/assert.h"
#include "utils/string_pool.hpp"
#include "version.hpp"
#include <cctype>
#include <cstdint>
#include <expected>
#include <format>
#include <sstream>
#include <string>
#include <vector>

namespace kepler {

    std::expected<void, Diagnostic> CmdParser::parse(int argc, char** argv) {
        if (argc == 1) {
            // No option given, just set help option to true if it exists
            const StringId help_id = StringPool::get().store("help");
            CmdOption* help_option = find_option(help_id);
            if (help_option != nullptr) {
                return help_option->set_value({"true"});
            }
            return {};
        }

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
                        .message = std::format("Unknown cmd option '--{}' at position {}", long_arg_name, index + 1),
                    });
                }
            } else if (arg.starts_with('-')) {
                const std::string short_arg_name_string = arg.substr(1);
                if (short_arg_name_string.size() != 1) {
                    return std::unexpected(Diagnostic{
                        .code = DiagnosticCode::InvalidOptionFormat,
                        .message = std::format("Lonely '-' as cmd option at position {}, it needs a name :'(", index + 1),
                    });
                }

                char short_arg_name = short_arg_name_string[0];
                if (!std::isalpha(short_arg_name)) {
                    return std::unexpected(Diagnostic{
                        .code = DiagnosticCode::InvalidOptionFormat,
                        .message = std::format("Short version of cmd option must be a character, received '{}'", short_arg_name),
                    });
                }

                option = find_option(short_arg_name);
                if (option == nullptr) {
                    return std::unexpected(Diagnostic{
                        .code = DiagnosticCode::UnknownOption,
                        .message = std::format("Unknown cmd option '-{}' at position {}", short_arg_name, index + 1),
                    });
                }
            }

            KPL_ASSERT_NOT_NULLPTR(option);
            if (option->option_defined) {
                return std::unexpected(Diagnostic{
                    .code = DiagnosticCode::OptionUsedTooOften,
                    .message = std::format("Cmd option '-{}/--{}' can only be used once",
                        option->short_name,
                        StringPool::get().lookup(option->long_name_id)),
                });
            }

            switch (option->value_type) {
                case CmdOptionValueType::NoValue: {
                    const auto set_result = option->set_value({"true"});
                    if (!set_result) {
                        return std::unexpected(set_result.error());
                    }
                    if (index < argc - 1) {
                        if (argv[index + 1][0] != '-') {
                            return std::unexpected(Diagnostic{
                                .code = DiagnosticCode::InvalidOptionValue,
                                .message = std::format("Cmd option '-{}'/'--{}' doesn't take a value",
                                    option->short_name,
                                    StringPool::get().lookup(option->long_name_id)),
                            });
                        }
                    }
                    break;
                }
                case CmdOptionValueType::OneValue: {
                    if (index >= argc - 1) {
                        return std::unexpected(Diagnostic{
                            .code = DiagnosticCode::MissingOptionValue,
                            .message = std::format("Cmd option '-{}'/'--{}' needs a value",
                                option->short_name,
                                StringPool::get().lookup(option->long_name_id)),
                        });
                    }

                    index += 1;
                    if (argv[index][0] == '-') {
                        return std::unexpected(Diagnostic{
                            .code = DiagnosticCode::MissingOptionValue,
                            .message = std::format("Cmd option '-{}'/'--{}' needs a value",
                                option->short_name,
                                StringPool::get().lookup(option->long_name_id)),
                        });
                    }
                    const auto set_result = option->set_value({std::string(argv[index])});
                    if (!set_result) {
                        return std::unexpected(set_result.error());
                    }

                    if (index + 1 <= argc - 1) {
                        if (argv[index + 1][0] != '-') {
                            return std::unexpected(Diagnostic{
                                .code = DiagnosticCode::InvalidOptionValue,
                                .message = std::format("Cmd option '-{}'/'--{}' only takes one value, received multiple values",
                                    option->short_name,
                                    StringPool::get().lookup(option->long_name_id)),
                            });
                        }
                    }
                    break;
                }
                case CmdOptionValueType::List: {
                    if (index >= argc - 1) {
                        return std::unexpected(Diagnostic{
                            .code = DiagnosticCode::MissingOptionValue,
                            .message = std::format("Cmd option '-{}'/'--{}' needs a value",
                                option->short_name,
                                StringPool::get().lookup(option->long_name_id)),
                        });
                    }

                    index += 1;
                    if (argv[index][0] == '-') {
                        return std::unexpected(Diagnostic{
                            .code = DiagnosticCode::MissingOptionValue,
                            .message = std::format("Cmd option '-{}'/'--{}' needs a value",
                                option->short_name,
                                StringPool::get().lookup(option->long_name_id)),
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
                    const auto set_result = option->set_value({args});
                    if (!set_result) {
                        return std::unexpected(set_result.error());
                    }
                    index -= 1; // Go back one because otherwise one argument will be skipped
                    break;
                }
            }

            option->option_defined = true;
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
            result += wrap_text_with_indentation(std::string(StringPool::get().lookup(option.description_id)), 80, "        ");
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

    std::string CmdParser::wrap_text_with_indentation(const std::string& text, uint32_t width, const std::string& indent) const {
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
                        current_width += word.size();
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
