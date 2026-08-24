// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "compiler.hpp"
#include "ast/abstract_syntax_tree.hpp"
#include "ast/ast_node.hpp"
#include "codegen/codegen_pass.hpp"
#include "codegen/optimizer.hpp"
#include "cxxopts.hpp"
#include "diagnostics/diagnostic.hpp"
#include "diagnostics/diagnostic_sink.hpp"
#include "io/file.hpp"
#include "lexer/token.hpp"
#include "lexer/tokenizer.hpp"
#include "parser/parser.hpp"
#include "semantic_analysis/name_resolution_pass.hpp"
#include "semantic_analysis/return_check_pass.hpp"
#include "semantic_analysis/symbol_table.hpp"
#include "type_system/type_check_pass.hpp"
#include "type_system/type_table.hpp"
#include "utils/ansi_codes.hpp"
#include "utils/assert.h"
#include "utils/ast_print_pass.hpp"
#include "utils/log.hpp"
#include <cstddef>
#include <cstdlib>
#include <expected>
#include <filesystem>
#include <format>
#include <llvm/IR/Module.h>
#include <memory>
#include <print>
#include <string>
#include <utility>
#include <vector>

namespace kepler {

    int Compiler::run(int argc, char** argv) const {
        DiagnosticSink diagnostic_sink;

        // Parse command line arguments
        const auto parse_result = parse_args(argc, argv);
        if (!parse_result) {
            const DiagnosticSeverity severity = get_diagnostic_severity(parse_result.error().code);
            std::println("{}{}", severity, parse_result.error().message);
            return EXIT_FAILURE;
        }
        const CompilerContext context = *parse_result;
        if (context.help_requested) {
            std::println("{}", context.help);
            return EXIT_SUCCESS;
        }

        log::config.should_log_verbose = context.log_verbose;
        log::verbose("Compiling project with the following context:\n{}-i (input file name): '{}'\n{}-o (output file name): '{}",
            log::indented,
            context.input_path.string(),
            log::last_indented,
            context.output_path.string());

        const auto file = File::load(context.input_file_path);
        const auto file = File::load(context.input_path);
        if (!file) {
            DiagnosticSeverity severity = get_diagnostic_severity(file.error().code);
            std::println("{}{}", severity, file.error().message);
            return EXIT_FAILURE;
        }

        SymbolTable symbol_table;
        TypeTable type_table;

        // AST creation
        Tokenizer tokenizer(*file, diagnostic_sink, type_table);
        std::vector<Token> tokens = tokenizer.tokenize();
        Parser parser(std::move(tokens), *file, diagnostic_sink, type_table);
        AbstractSyntaxTree ast = parser.parse();
        verify_ast(ast);
        ASTPrintPass ast_print_pass(ast);
        // ast_print_pass.run();

        // AST passes
        // Currently there are only builtin types so there is no need to do any type resolution before name resolution
        // Once user defined types are implemented the first pass should be a type resolution pass which creates the type objects for all user defined types
        ReturnCheckPass return_check_pass(ast, diagnostic_sink);
        return_check_pass.run();
        NameResolutionPass name_resolution_pass(ast, diagnostic_sink, symbol_table, type_table);
        name_resolution_pass.run();
        TypeCheckPass type_check_pass(ast, diagnostic_sink, type_table);
        type_check_pass.run();
        // ast_print_pass.run();

        // Print diagnostics and abort if any of the passes encountered errors
        if (diagnostic_sink.get_error_count() > 0) {
            diagnostic_sink.flush();
            std::println("{}{}[ This one's on you ]{}: Compilation failed with {} error(s) and {} warning(s){}",
                ansi_codes::bold,
                ansi_codes::bg_red,
                ansi_codes::reset_bold_and_dim,
                diagnostic_sink.get_warning_count(),
                diagnostic_sink.get_error_count(),
                ansi_codes::reset);
            return EXIT_FAILURE;
        }

        // Do the actual code generation and compilation
        CodegenPass codegen_pass(ast, symbol_table, type_table);
        const std::unique_ptr<llvm::Module> llvm_module = codegen_pass.run();
        if (llvm_module == nullptr) {
            return EXIT_FAILURE;
        }
        optimize_module(llvm_module);

        // Print final compilation result
        if (diagnostic_sink.get_warning_count() > 0) {
            diagnostic_sink.flush();
            std::println("{}{}{}[ There's room for improvement ]{}: Compilation succeeded, but with {} warning(s){}",
                ansi_codes::bold,
                ansi_codes::black,
                ansi_codes::bg_yellow,
                ansi_codes::reset_bold_and_dim,
                diagnostic_sink.get_warning_count(),
                ansi_codes::reset);
        } else {
            std::println("{}{}{}[ You're a god damn genius ]{}: Compilation succeeded{}",
                ansi_codes::bold,
                ansi_codes::black,
                ansi_codes::bg_green,
                ansi_codes::reset_bold_and_dim,
                ansi_codes::reset);
        }

        return EXIT_SUCCESS;
    }

    void Compiler::verify_ast(const AbstractSyntaxTree& ast) const {
        for (const std::unique_ptr<ASTNode>& node : ast.top_level_nodes) {
            bool is_valid_top_level_node = node->node_type == ASTNodeType::Extern || node->node_type == ASTNodeType::Function;
            KPL_ASSERT_THAT(is_valid_top_level_node, "Malformed ast with node of type '{}' on top level", node->node_type);
        }
    }

    std::expected<CompilerContext, Diagnostic> Compiler::parse_args(int argc, char** argv) const {
        try {
            cxxopts::Options options("kepler", "The compiler for the kepler programming language");
            // clang-format off
            options.add_options()
                ("i,input", "The .kpl input file", cxxopts::value<std::string>())
                ("o,output", "The output file", cxxopts::value<std::string>())
                ("v,verbose", "Enable verbose logging", cxxopts::value<bool>())
                ("h,help", "Print help");
            // clang-format on
            cxxopts::ParseResult parse_result = options.parse(argc, argv);

            CompilerContext context;
            // Input option
            const int input_count = parse_result.count("input");
            if (input_count == 1) {
                context.input_path = parse_result["input"].as<std::string>();
                const std::string extension = context.input_path.extension();
                if (extension != ".kpl") {
                    const std::string message = std::format("Input file (-i) must be a '.kpl' file, received '{}'", extension);
                    return std::unexpected(Diagnostic{.code = DiagnosticCode::WrongFileFormat, .message = message});
                }
            } else if (input_count > 1) {
                const std::string message = std::format("Input file (-i) can only be specified once, but was specified {} times", input_count);
                return std::unexpected(Diagnostic{.code = DiagnosticCode::OptionUsedTooOften, .message = message});
            } else {
                return std::unexpected(Diagnostic{.code = DiagnosticCode::NoInputFile, .message = "Missing input file (-i)"});
            }

            // Output option
            const int output_count = parse_result.count("output");
            if (output_count == 1) {
                context.output_path = parse_result["output"].as<std::string>();
            } else if (input_count > 1) {
                const std::string message = std::format("Output file (-o) can only be specified once, but was specified {} times", output_count);
                return std::unexpected(Diagnostic{.code = DiagnosticCode::OptionUsedTooOften, .message = message});
            } else {
                return std::unexpected(Diagnostic{.code = DiagnosticCode::NoOutputFile, .message = "Missing output file (-o)"});
            }

            context.log_verbose = parse_result.contains("verbose");
            context.help_requested = parse_result.contains("help");
            if (context.help_requested) {
                context.help = options.help();
            }

            return context;
        } catch (const cxxopts::exceptions::exception& e) {
            return std::unexpected(Diagnostic{.code = DiagnosticCode::UnknownOption, .message = e.what()});
        }
    }
}
