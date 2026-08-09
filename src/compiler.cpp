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
#include "ast/ast_printer.hpp"
#include "codegen/codegen_pass.hpp"
#include "cxxopts.hpp"
#include "diagnostics/diagnostic.hpp"
#include "diagnostics/diagnostic_sink.hpp"
#include "io/file.hpp"
#include "lexer/tokenizer.hpp"
#include "parser/parser.hpp"
#include "semantic_analysis/name_resolution_pass.hpp"
#include "semantic_analysis/symbol_table.hpp"
#include "type_system/type_check_pass.hpp"
#include "type_system/type_table.hpp"
#include "utils/log.hpp"
#include <cassert>
#include <cstdlib>
#include <expected>
#include <filesystem>
#include <memory>
#include <print>
#include <string>

namespace kepler {

    int Compiler::run(int argc, char** argv) const {
        DiagnosticSink diagnostic_sink;

        // Parse command line arguments
        const auto parse_result = parse_args(argc, argv);
        if (!parse_result) {
            diagnostic_sink.report(parse_result.error().code, parse_result.error().message);
            diagnostic_sink.flush();
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
            context.input_file_path,
            log::last_indented,
            context.input_file_path);

        const auto file = File::load(context.input_file_path);
        if (!file) {
            DiagnosticSeverity severity = get_diagnostic_severity(file.error().code);
            std::println("{}{}", severity, file.error().message);
            return EXIT_FAILURE;
        }

        SymbolTable symbol_table;
        TypeTable type_table;

        // AST creation
        Tokenizer tokenizer(*file, diagnostic_sink, type_table);
        Parser parser(tokenizer.tokenize(), *file, diagnostic_sink, type_table);
        AbstractSyntaxTree ast = parser.parse();
        verify_ast(ast);
        ASTPrinter ast_printer(ast);
        ast_printer.run();

        // AST passes
        // Currently there are only builtin types so there is no need to do any type resolution before name resolution
        // Once user defined types are implemented the first pass should be a type resolution pass which creates the type objects for all user defined types
        NameResolutionPass name_resolution_pass(ast, diagnostic_sink, symbol_table, type_table);
        name_resolution_pass.run();
        ast_printer.run();
        TypeCheckPass type_check_pass(ast, diagnostic_sink, symbol_table, type_table);
        type_check_pass.run();

        // Print diagnostics and abort if any of the passes encountered errors
        if (diagnostic_sink.get_error_count() > 0) {
            diagnostic_sink.flush();
            std::println("{}{}[ This one's on you ]{}{}: Compilation failed with {} error(s) and {} warning(s){}",
                ansi_codes::bold,
                ansi_codes::bg_red,
                ansi_codes::reset,
                ansi_codes::bg_red,
                diagnostic_sink.get_error_count(),
                diagnostic_sink.get_warning_count(),
                ansi_codes::reset);
            return EXIT_FAILURE;
        }

        // Do the actual code generation and compilation
        CodegenPass codegen_pass(ast);
        codegen_pass.run();

        return EXIT_SUCCESS;
    }

    void Compiler::verify_ast(const AbstractSyntaxTree& ast) const {
#if NDEBUG
        return;
#else
        for (const std::unique_ptr<ASTNode>& node : ast.top_level_nodes) {
            assert(node->node_type == ASTNodeType::Extern || node->node_type == ASTNodeType::Function && "Invalid ast node type on top level");
        }
#endif
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
                context.input_file_path = parse_result["input"].as<std::string>();
            } else if (input_count > 1) {
                const std::string message = std::format("Input file (-i) can only be specified once, but was specified {} times", input_count);
                return std::unexpected(Diagnostic{.code = DiagnosticCode::TooManyInputFiles, .message = message});
            } else {
                return std::unexpected(Diagnostic{.code = DiagnosticCode::NoInputFile, .message = "Missing input file (-i)"});
            }

            // Output option
            const int output_count = parse_result.count("output");
            if (output_count == 1) {
                context.output_file_path = parse_result["output"].as<std::string>();
            } else if (input_count > 1) {
                const std::string message = std::format("Output file (-o) can only be specified once, but was specified {} times", output_count);
                return std::unexpected(Diagnostic{.code = DiagnosticCode::TooManyOutputFiles, .message = message});
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
            return std::unexpected(Diagnostic{.code = DiagnosticCode::CxxoptsException, .message = e.what()});
        }
    }

}
