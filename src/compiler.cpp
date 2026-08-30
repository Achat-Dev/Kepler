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
#include "version.hpp"
#include <cstddef>
#include <cstdlib>
#include <expected>
#include <filesystem>
#include <format>
#include <llvm/CodeGen/MachineFunction.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/CodeGen.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/TargetParser/Triple.h>
#include <memory>
#include <print>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace kepler {

    int Compiler::run(int argc, char** argv) const {
        const auto dependency_result = do_dependencies_exist();
        if (!dependency_result) {
            print_diagnostic(dependency_result.error());
            return EXIT_FAILURE;
        }

        DiagnosticSink diagnostic_sink;

        // Parse command line arguments
        const auto parse_result = parse_args(argc, argv);
        if (!parse_result) {
            print_diagnostic(parse_result.error());
            return EXIT_FAILURE;
        }
        const CompilerContext context = *parse_result;
        if (context.help_requested) {
            std::println("{}", context.help);
            return EXIT_SUCCESS;
        }
        if (context.version_requested) {
            std::println("kepler version {}.{}.{}", version.major, version.minor, version.patch);
            return EXIT_SUCCESS;
        }

        // Load file to start compilation
        const auto file = File::load(context.input_path);
        if (!file) {
            print_diagnostic(file.error());
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
        ReturnCheckPass return_check_pass(ast, diagnostic_sink, type_table);
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
                diagnostic_sink.get_error_count(),
                diagnostic_sink.get_warning_count(),
                ansi_codes::reset);
            return EXIT_FAILURE;
        }

        // Do the actual code generation and compilation
        CodegenPass codegen_pass(ast, symbol_table, type_table);
        const std::unique_ptr<llvm::Module> llvm_module = codegen_pass.run();
        if (llvm_module == nullptr) {
            return EXIT_FAILURE;
        }

        llvm::TargetMachine* target_machine = create_target_machine();
        if (target_machine == nullptr) {
            return EXIT_FAILURE;
        }
        // It's important for optimization to set the data layout before running the optimizer
        llvm_module->setTargetTriple(target_machine->getTargetTriple());
        llvm_module->setDataLayout(target_machine->createDataLayout());
        optimize_module(llvm_module, context.optimization_level);

        // TOOD (bug): This works for now, but when multiple input files are introduced the object files will always override each other
        std::filesystem::path object_output_path = context.output_path;
        object_output_path.replace_extension("o");
        const bool object_code_emission_successful = emit_object_code(llvm_module, target_machine, object_output_path);
        if (!object_code_emission_successful) {
            return EXIT_FAILURE;
        }

        const bool executable_linking_successful = link_to_executable(object_output_path,
            context.additional_paths,
            context.optimization_level,
            context.output_path);
        if (!executable_linking_successful) {
            return EXIT_FAILURE;
        }

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

    std::expected<void, Diagnostic> Compiler::do_dependencies_exist() const {
#ifdef _WIN32
        bool does_clang_exist = std::system("where clang >nul 2>&1") == 0;
#else
        bool does_clang_exist = std::system("command -v clang >/dev/null 2>&1") == 0;
#endif
        if (!does_clang_exist) {
            return std::unexpected(Diagnostic{.code = DiagnosticCode::MissingDependency, .message = "'clang' not found, which is needed for the compilation process. Install clang and add it to PATH (https://github.com/llvm/llvm-project/releases)"});
        }
        return {};
    }

    std::expected<CompilerContext, Diagnostic> Compiler::parse_args(int argc, char** argv) const {
        try {
            cxxopts::Options options("kepler", "The compiler for the kepler programming language");
            // clang-format off
            options.add_options()
                ("i,input", "The .kpl input file", cxxopts::value<std::string>())
                ("o,output", "The output file", cxxopts::value<std::string>())
                ("a,additional-files",
                    "Additional .c or .o files, separated by ','\n"
                        "CAUTION: These inputs are not sanitized and are passed directly to a shell command as is."
                    , cxxopts::value<std::vector<std::string>>())
                ("O,optimization-level",
                    "The optimization level to use. Possible values for arg:\n"
                        "- 0: No optimization\n"
                        "- 1: Optimize quickly without destroying debuggability\n"
                        "- 2: Optimize for fast execution as much as possible without triggering significant incremental compile time or code size growth\n"
                        "- 3: Optimize for fast execution as much as possible no matter the compilation cost\n"
                        "- s: Similar to 2 but tries to optimize for small code size instead of fast execution\n"
                        "- z: A very specialized mode that will optimize for code size at any and all costs",
                    cxxopts::value<std::string>())
                ("v,version", "Print the compiler version", cxxopts::value<bool>())
                ("h,help", "Print help", cxxopts::value<bool>());
            // clang-format on
            cxxopts::ParseResult parse_result = options.parse(argc, argv);

            CompilerContext context;

            // Help
            context.help_requested = parse_result.contains("help");
            if (context.help_requested) {
                context.help = options.help();
                return context;
            }

            // Version
            context.version_requested = parse_result.contains("version");
            if (context.version_requested) {
                return context;
            }

            // Input file
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

            // Output file
            const int output_count = parse_result.count("output");
            if (output_count == 1) {
                context.output_path = parse_result["output"].as<std::string>();
            } else if (output_count > 1) {
                const std::string message = std::format("Output file (-o) can only be specified once, but was specified {} times", output_count);
                return std::unexpected(Diagnostic{.code = DiagnosticCode::OptionUsedTooOften, .message = message});
            } else {
                return std::unexpected(Diagnostic{.code = DiagnosticCode::NoOutputFile, .message = "Missing output file (-o)"});
            }

            // Additional files
            const int additional_count = parse_result.count("additional-files");
            if (additional_count == 1) {
                const std::vector<std::string> additional_paths = parse_result["additional-files"].as<std::vector<std::string>>();
                context.additional_paths.reserve(additional_paths.size());
                for (size_t i = 0; i < additional_paths.size(); i++) {
                    context.additional_paths.push_back(additional_paths[i]);
                    const std::string extension = context.additional_paths[i].extension();
                    if (extension != ".o" && extension != ".c") {
                        const std::string message = std::format("Additional files can only be '.c' and '.o' files, received '{}'", extension);
                        return std::unexpected(Diagnostic{.code = DiagnosticCode::WrongFileFormat, .message = message});
                    }
                }
            } else if (additional_count > 1) {
                const std::string message = std::format("Additional files (-a) can only be specified once, but was specified {} times", additional_count);
                return std::unexpected(Diagnostic{.code = DiagnosticCode::OptionUsedTooOften, .message = message});
            }

            // Optimization level
            const int optimization_level_count = parse_result.count("optimization-level");
            if (optimization_level_count > 1) {
                const std::string message = std::format("Optimization level (-O) can only be specified once, but was specified {} times",
                    optimization_level_count);
                return std::unexpected(Diagnostic{.code = DiagnosticCode::OptionUsedTooOften, .message = message});
            } else {
                std::string optimization_level = "2";
                if (optimization_level_count == 1) {
                    optimization_level = parse_result["optimization-level"].as<std::string>();
                }
                if (optimization_level == "0") {
                    context.optimization_level = OptimizationLevel::O0;
                } else if (optimization_level == "1") {
                    context.optimization_level = OptimizationLevel::O1;
                } else if (optimization_level == "2") {
                    context.optimization_level = OptimizationLevel::O2;
                } else if (optimization_level == "3") {
                    context.optimization_level = OptimizationLevel::O3;
                } else if (optimization_level == "s") {
                    context.optimization_level = OptimizationLevel::Os;
                } else if (optimization_level == "z") {
                    context.optimization_level = OptimizationLevel::Oz;
                } else {
                    const std::string message = std::format("Unknown optimization level '{}', available options are 0, 1, 2, 3, s and z", optimization_level);
                    return std::unexpected(Diagnostic{.code = DiagnosticCode::UnknownOptimizationLevel, .message = message});
                }
            }

            return context;
        } catch (const cxxopts::exceptions::exception& e) {
            return std::unexpected(Diagnostic{.code = DiagnosticCode::UnknownOption, .message = e.what()});
        }
    }

    void Compiler::verify_ast(const AbstractSyntaxTree& ast) const {
        for (const std::unique_ptr<ASTNode>& node : ast.top_level_nodes) {
            bool is_valid_top_level_node = node->node_type == ASTNodeType::Extern || node->node_type == ASTNodeType::Function;
            KPL_ASSERT_THAT(is_valid_top_level_node, "Malformed ast with node of type '{}' on top level", node->node_type);
        }
    }

    llvm::TargetMachine* Compiler::create_target_machine() const {
        llvm::InitializeAllTargetInfos();
        llvm::InitializeAllTargets();
        llvm::InitializeAllTargetMCs();
        llvm::InitializeAllAsmParsers();
        llvm::InitializeAllAsmPrinters();

        const std::string target_triple_string = llvm::sys::getDefaultTargetTriple();
        llvm::Triple target_triple(target_triple_string);

        std::string error;
        const llvm::Target* target = llvm::TargetRegistry::lookupTarget(target_triple, error);
        if (target == nullptr) {
            log::error("Failed to lookup target triple:\n{}", error);
            return nullptr;
        }

        const char* cpu = "generic";
        const char* features = "";
        llvm::TargetOptions target_options;
        llvm::TargetMachine* target_machine = target->createTargetMachine(target_triple, cpu, features, target_options, llvm::Reloc::PIC_);
        if (target_machine == nullptr) {
            log::error("Failed to create target machine");
            return nullptr;
        }
        return target_machine;
    }

    // clang-format off
    bool Compiler::emit_object_code(const std::unique_ptr<llvm::Module>& module,
        llvm::TargetMachine* target_machine,
        const std::filesystem::path& output_path) const
    {
        // clang-format on
        KPL_ASSERT_NOT_NULLPTR(module);
        KPL_ASSERT_NOT_NULLPTR(target_machine);
        KPL_ASSERT_THAT(!output_path.empty(), "Output path must not be empty for object code emission");
        KPL_ASSERT_THAT(output_path.extension() == ".o", "Output path must end with '.o' for object code emission");
        std::error_code error_code;
        llvm::raw_fd_ostream out_stream(output_path.string(), error_code, llvm::sys::fs::OF_None);
        if (error_code) {
            log::error("Failed to open output file '{}':\n{}", output_path.string(), error_code.message());
            out_stream.flush();
            return false;
        }

        llvm::legacy::PassManager pass_manager;
        const bool pass_creation_failed = target_machine->addPassesToEmitFile(pass_manager, out_stream, nullptr, llvm::CodeGenFileType::ObjectFile);
        if (pass_creation_failed) {
            log::error("Failed to create object code emission pass");
            out_stream.flush();
            return false;
        }

        pass_manager.run(*module);
        out_stream.flush();
        return true;
    }

    // TODO (fix): This is highly unsafe because the user input is not sanitized
    // clang-format off
    bool Compiler::link_to_executable(const std::filesystem::path& object_path,
        const std::vector<std::filesystem::path>& additional_paths,
        OptimizationLevel optimization_level,
        const std::filesystem::path& output_path) const
    {
        // clang-format on
        KPL_ASSERT_THAT(!object_path.empty(), "Object path must not be empty for executable linking");
        KPL_ASSERT_THAT(object_path.extension() == ".o", "Object path must have '.o' as the file extension for executable linking");
        KPL_ASSERT_THAT(!output_path.empty(), "Output path must not be empty for executable linking");
        std::string command = "clang \"" + object_path.string() + "\" ";
        for (const std::filesystem::path& additional_path : additional_paths) {
            KPL_ASSERT_THAT(additional_path.extension() == ".c" || additional_path.extension() == ".o",
                "Additional files must have either '.c' or '.o' as the file extension for executable linking");
            if (!std::filesystem::exists(additional_path)) {
                log::error("Additional file path '{}' doesn't exist", additional_path.string());
                return false;
            }
            command += " \"" + additional_path.string() + "\" ";
        }
        command += std::format(" -{} -o \"{}\"", optimization_level, output_path.string());
        const int command_result = std::system(command.c_str());
        if (command_result == 0) {
            return true;
        } else {
            return false;
        }
    }

    void Compiler::print_diagnostic(const Diagnostic& diagnostic) const {
        const DiagnosticSeverity severity = get_diagnostic_severity(diagnostic.code);
        std::println("{}{}", severity, diagnostic.message);
    }

}
