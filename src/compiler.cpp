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
#include "diagnostics/diagnostic.hpp"
#include "diagnostics/diagnostic_sink.hpp"
#include "io/file.hpp"
#include "io/file_manager.hpp"
#include "lexer/token.hpp"
#include "lexer/tokenizer.hpp"
#include "parser/parser.hpp"
#include "semantic_analysis/module_creation_pass.hpp"
#include "semantic_analysis/name_resolution_pass.hpp"
#include "semantic_analysis/return_check_pass.hpp"
#include "semantic_analysis/symbol_table.hpp"
#include "type_system/type_check_pass.hpp"
#include "type_system/type_table.hpp"
#include "utils/assert.h"
#include "utils/cmd_parser.hpp"
#include "utils/log.hpp"
#include "version.hpp"
#include <cstdlib>
#include <cstring>
#include <expected>
#include <filesystem>
#include <format>
#include <llvm/CodeGen/MachineFunction.h>
#include <llvm/IR/LLVMContext.h>
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
#include <optional>
#include <print>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#ifdef _WIN32
#include <process.h>
#else
#include <spawn.h>
#include <sys/types.h>
#include <sys/wait.h>

extern char** environ;
#endif

namespace kepler {

    int Compiler::run(int argc, char** argv) const {
        const auto dependency_result = do_dependencies_exist();
        if (!dependency_result) {
            print_diagnostic(dependency_result.error());
            return EXIT_FAILURE;
        }

        // Parse command line arguments
        const auto args_parse_result = parse_args(argc, argv);
        if (!args_parse_result) {
            print_diagnostic(args_parse_result.error());
            return EXIT_FAILURE;
        }
        const CmdArgs cmd_args = *args_parse_result;
        if (cmd_args.help_requested) {
            std::println("{}", cmd_args.help);
            return EXIT_SUCCESS;
        }
        if (cmd_args.version_requested) {
            std::println("kepler version {}.{}.{}", version.major, version.minor, version.patch);
            return EXIT_SUCCESS;
        }

        DiagnosticSink diagnostic_sink;
        SymbolTable symbol_table;
        TypeTable type_table;
        // Important: The llvm context has to have the same lifetime as the llvm modules, that's why we declare it here
        llvm::LLVMContext llvm_context;

        auto asts = create_asts(cmd_args.input_paths, diagnostic_sink, type_table);
        if (!asts) {
            return EXIT_FAILURE;
        }

        llvm::TargetMachine* target_machine = create_target_machine();
        if (!target_machine) {
            return EXIT_FAILURE;
        }

        auto llvm_modules = run_ast_passes(*asts, diagnostic_sink, symbol_table, type_table, llvm_context, target_machine, cmd_args.optimization_level);
        if (!llvm_modules) {
            return EXIT_FAILURE;
        }

        const bool compilation_successul = create_executable(*llvm_modules,
            target_machine,
            cmd_args.additional_paths,
            cmd_args.optimization_level,
            cmd_args.output_path);
        if (!compilation_successul) {
            return EXIT_FAILURE;
        }

        // Print final compilation result
        KPL_ASSERT_THAT(diagnostic_sink.get_error_count() == 0);
        diagnostic_sink.flush();

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

    std::expected<CmdArgs, Diagnostic> Compiler::parse_args(int argc, char** argv) const {
        CmdArgs cmd_args;
        std::string optimization_level_string = "2";
        CmdParser cmd_parser("The compiler for the kepler programming language");
        cmd_parser.add_option(&cmd_args.input_paths, 'i', "input", "The .kpl input file");
        cmd_parser.add_option(&cmd_args.output_path, 'o', "output", "The output file");
        cmd_parser.add_option(&cmd_args.additional_paths, 'a', "additional-files",
            "Additional .c or .o files, separated by spaces");
        cmd_parser.add_option(&optimization_level_string, 'O', "optimization-level",
            "The optimization level to use. Possible values for <arg>:\n"
            "- 0: (Almost) no optimization\n"
            "- 1: Optimize quickly without destroying debuggability\n"
            "- 2: (Default value) Optimize for fast execution as much as possible without triggering significant incremental compile time or code size growth\n"
            "- 3: Optimize for fast execution as much as possible no matter the compilation cost\n"
            "- s: Similar to 2 but tries to optimize for small code size instead of fast execution\n"
            "- z: A very specialized mode that will optimize for code size at any and all costs");
        cmd_parser.add_option(&cmd_args.version_requested, 'v', "version", "Print the compiler version");
        cmd_parser.add_option(&cmd_args.help_requested, 'h', "help", "Print help");
        const auto parse_result = cmd_parser.parse(argc, argv);
        if (!parse_result) {
            return std::unexpected(parse_result.error());
        }

        // Help
        if (cmd_args.help_requested) {
            cmd_args.help = cmd_parser.get_help();
            return cmd_args;
        }

        // Version
        if (cmd_args.version_requested) {
            return cmd_args;
        }

        // Input file
        if (cmd_args.input_paths.empty()) {
            return std::unexpected(Diagnostic{.code = DiagnosticCode::NoInputFile, .message = "Missing input file (-i)"});
        } else {
            for (const std::filesystem::path& input_path : cmd_args.input_paths) {
                const std::filesystem::path extension = input_path.extension();
                if (extension != ".kpl") {
                    const std::string message = std::format("Input file (-i) must be a '.kpl' file, received '{}'", extension.string());
                    return std::unexpected(Diagnostic{.code = DiagnosticCode::WrongFileFormat, .message = message});
                }
            }
        }

        // Output file
        if (cmd_args.output_path.empty()) {
            return std::unexpected(Diagnostic{.code = DiagnosticCode::NoOutputFile, .message = "Missing output file (-o)"});
        }

        // Additional files
        if (!cmd_args.additional_paths.empty()) {
            for (const std::filesystem::path& additional_path : cmd_args.additional_paths) {
                const std::filesystem::path extension = additional_path.extension();
                if (extension != ".c" && extension != ".o") {
                    const std::string message = std::format("Additional files can only be '.c' and '.o' files, received '{}'", extension.string());
                    return std::unexpected(Diagnostic{.code = DiagnosticCode::WrongFileFormat, .message = message});
                }
            }
        }

        // Optimization level
        if (optimization_level_string == "0") {
            cmd_args.optimization_level = OptimizationLevel::O0;
        } else if (optimization_level_string == "1") {
            cmd_args.optimization_level = OptimizationLevel::O1;
        } else if (optimization_level_string == "2") {
            cmd_args.optimization_level = OptimizationLevel::O2;
        } else if (optimization_level_string == "3") {
            cmd_args.optimization_level = OptimizationLevel::O3;
        } else if (optimization_level_string == "s") {
            cmd_args.optimization_level = OptimizationLevel::Os;
        } else if (optimization_level_string == "z") {
            cmd_args.optimization_level = OptimizationLevel::Oz;
        } else {
            const std::string message = std::format("Unknown optimization level '{}', available values are 0, 1, 2, 3, s and z", optimization_level_string);
            return std::unexpected(Diagnostic{.code = DiagnosticCode::UnknownOptimizationLevel, .message = message});
        }

        return cmd_args;
    }

    // clang-format off
    std::optional<std::vector<AbstractSyntaxTree>> Compiler::create_asts(const std::vector<std::filesystem::path> file_paths,
        DiagnosticSink& diagnostic_sink,
        TypeTable& type_table) const
    {
        // clang-format on
        KPL_ASSERT_THAT(!file_paths.empty());
        bool all_files_found = true;
        std::vector<AbstractSyntaxTree> asts;
        for (const std::filesystem::path file_path : file_paths) {
            KPL_ASSERT_THAT(file_path.extension() == ".kpl", "Required extension: '.kpl', received: '{}'", file_path.extension().string());
            const auto file = FileManager::get().load(file_path);
            if (!file) {
                print_diagnostic(file.error());
                all_files_found = false;
                continue;
            }

            Tokenizer tokenizer(*file, diagnostic_sink, type_table);
            std::vector<Token> tokens = tokenizer.tokenize();
            Parser parser(*file, std::move(tokens), diagnostic_sink, type_table);
            AbstractSyntaxTree ast = parser.parse();
            verify_ast(ast, *file);
            asts.push_back(std::move(ast));
        }

        if (!all_files_found) {
            return std::nullopt;
        }
        return asts;
    }

    void Compiler::verify_ast(AbstractSyntaxTree& ast, const File* file) const {
        KPL_ASSERT_NOT_NULLPTR(file);
        KPL_ASSERT_NOT_NULLPTR(ast.module_statement);
        for (const std::unique_ptr<ASTNode>& node : ast.top_level_nodes) {
            bool is_valid_top_level_node = node->node_type == ASTNodeType::Extern || node->node_type == ASTNodeType::Function;
            KPL_ASSERT_THAT(is_valid_top_level_node, "Malformed ast with node of type '{}' on top level", node->node_type);
        }
    }

    // clang-format off
    std::optional<std::vector<std::unique_ptr<llvm::Module>>> Compiler::run_ast_passes(std::vector<AbstractSyntaxTree>& asts,
        DiagnosticSink& diagnostic_sink,
        SymbolTable& symbol_table,
        TypeTable& type_table,
        llvm::LLVMContext& llvm_context,
        llvm::TargetMachine* target_machine,
        OptimizationLevel optimization_level) const
    {
        // clang-format on
        KPL_ASSERT_THAT(!asts.empty());
        KPL_ASSERT_NOT_NULLPTR(target_machine);
        ReturnCheckPass return_check_pass(diagnostic_sink, type_table);
        return_check_pass.run(asts);
        ModuleCreationPass module_creation_pass(diagnostic_sink, symbol_table, type_table);
        module_creation_pass.run(asts);
        NameResolutionPass name_resolution_pass(diagnostic_sink, symbol_table, type_table);
        name_resolution_pass.run(asts);
        TypeCheckPass type_check_pass(diagnostic_sink, symbol_table, type_table);
        type_check_pass.run(asts);

        // Print diagnostics and abort if any of the passes encountered errors
        if (diagnostic_sink.get_error_count() > 0) {
            diagnostic_sink.flush();
            return std::nullopt;
        }

        CodegenPass codegen_pass(diagnostic_sink, symbol_table, type_table, llvm_context, target_machine, optimization_level);
        auto llvm_modules = codegen_pass.run(asts);
        if (!llvm_modules) {
            return std::nullopt;
        }

        if (diagnostic_sink.get_error_count() > 0) {
            diagnostic_sink.flush();
            return std::nullopt;
        }
        return std::move(*llvm_modules);
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
    bool Compiler::create_executable(std::vector<std::unique_ptr<llvm::Module>>& llvm_modules,
        llvm::TargetMachine* target_machine,
        const std::vector<std::filesystem::path>& additional_paths,
        OptimizationLevel optimization_level,
        const std::filesystem::path& output_path) const
    {
        // clang-format on
        KPL_ASSERT_THAT(!llvm_modules.empty());
        KPL_ASSERT_NOT_NULLPTR(target_machine);
        KPL_ASSERT_THAT(!output_path.empty());
        std::vector<std::filesystem::path> object_paths;
        for (std::unique_ptr<llvm::Module>& llvm_module : llvm_modules) {
            std::filesystem::path object_path = output_path;
            object_path.replace_filename(llvm_module->getModuleIdentifier() + ".o");
            const bool object_code_emission_successful = emit_object_code(llvm_module, target_machine, object_path);
            if (!object_code_emission_successful) {
                return false;
            }
            object_paths.push_back(std::move(object_path));
        }

        const bool executable_linking_successful = link_to_executable(object_paths,
            additional_paths,
            optimization_level,
            output_path);
        if (!executable_linking_successful) {
            return false;
        }

        return true;
    }

    // clang-format off
    bool Compiler::emit_object_code(const std::unique_ptr<llvm::Module>& module,
        llvm::TargetMachine* target_machine,
        const std::filesystem::path& output_path) const
    {
        // clang-format on
        KPL_ASSERT_NOT_NULLPTR(module);
        KPL_ASSERT_NOT_NULLPTR(target_machine);
        KPL_ASSERT_THAT(!output_path.empty());
        KPL_ASSERT_THAT(output_path.extension() == ".o", "Required extension: '.o', received: '{}'", output_path.extension().string());
        std::error_code error_code;
        llvm::raw_fd_ostream out_stream(output_path.string(), error_code, llvm::sys::fs::OF_None);
        if (error_code) {
            log::error("Failed to write object file '{}':\n{}{}", output_path.string(), log::last_indented, error_code.message());
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

    // clang-format off
    bool Compiler::link_to_executable(const std::vector<std::filesystem::path>& object_paths,
        const std::vector<std::filesystem::path>& additional_paths,
        OptimizationLevel optimization_level,
        const std::filesystem::path& output_path) const
    {
        // clang-format on
        KPL_ASSERT_THAT(!object_paths.empty());
        KPL_ASSERT_THAT(!output_path.empty());

        // Construct arguments
        std::vector<std::string> args;
#ifdef _WIN32
        args.push_back("clang.exe");
#else
        args.push_back("clang");
#endif
        // Add object paths to arguments
        for (const std::filesystem::path& object_path : object_paths) {
            KPL_ASSERT_THAT(object_path.extension() == ".o", "Required extension: '.o', received: '{}'", object_path.extension().string());
            if (!std::filesystem::exists(object_path)) {
                log::error("Object file path '{}' doesn't exist", object_path.string());
                return false;
            }
            args.push_back(object_path.string());
        }

        // Add additional paths to arguments
        for (const std::filesystem::path& additional_path : additional_paths) {
            KPL_ASSERT_THAT(additional_path.extension() == ".c" || additional_path.extension() == ".o",
                "Required extension: '.c' or '.o', received: '{}'",
                additional_path.extension().string());
            if (!std::filesystem::exists(additional_path)) {
                log::error("Additional file path '{}' doesn't exist", additional_path.string());
                return false;
            }
            args.push_back(additional_path.string());
        }
        args.push_back(std::format("-{}", optimization_level));
        args.push_back("-o");
        args.push_back(output_path.string());

        std::vector<char*> argv;
        argv.reserve(args.size());
        for (const std::string& arg : args) {
            argv.push_back(const_cast<char*>(arg.c_str()));
        }
        // Important: spawn functions expect the argument list to be null terminated
        argv.push_back(nullptr);

        // Execute shell command
#ifdef _WIN32
        int result = _spawnvp(_P_WAIT, argv[0], argv.data());
        if (result == -1) {
            const std::string error_message = std::format("{}Failed to start the linker", DiagnosticSeverity::Error);
            perror(error_message.c_str());
            return false;
        } else if (result != 0) {
            log::error("Failed to link executable");
            return false;
        }
        return true;
#else
        pid_t pid;
        const int return_value = posix_spawnp(&pid, argv[0], nullptr, nullptr, argv.data(), environ);
        if (return_value != 0) {
            log::error("Failed to start the linker: {}", strerror(return_value));
            return false;
        }

        int status;
        if (waitpid(pid, &status, 0) == -1) {
            const std::string error_message = std::format("{}Failed to wait for the linker process", DiagnosticSeverity::Error);
            perror(error_message.c_str());
            return false;
        }

        if (WIFEXITED(status)) {
            if (WEXITSTATUS(status) != 0) {
                log::error("Failed to link executable");
                return false;
            }
            return true;
        }

        if (WIFSIGNALED(status)) {
            int signal = WTERMSIG(status);
            log::error("Compilation process terminated with signal {} ({})", signal, strsignal(signal));
            return false;
        }

        log::error("I have no idea what went wrong, but something did when calling trying to link the executable");
        return false;
#endif
    }

    void Compiler::print_diagnostic(const Diagnostic& diagnostic) const {
        const DiagnosticSeverity severity = get_diagnostic_severity(diagnostic.code);
        std::println("{}{}", severity, diagnostic.message);
    }

}
