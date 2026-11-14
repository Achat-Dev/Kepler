// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2026 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "diagnostics/diagnostic.hpp"
#include "utils/assert.h"

namespace kepler {

    DiagnosticSeverity get_diagnostic_severity(DiagnosticCode diagnostic_code) {
        switch (diagnostic_code) {
            // -- Lexer --
            case DiagnosticCode::MultilineCommentNotClosed:
            // -- Module stuff --
            case DiagnosticCode::ImportOfSelf:
            case DiagnosticCode::RedundantImport:
            // -- Miscellaneous --
            case DiagnosticCode::UnreachableCode:
            case DiagnosticCode::RedundantCast:
                return DiagnosticSeverity::Warning;

            // -- Usage --
            // Cmd options
            case DiagnosticCode::UnknownCmdOption:
            case DiagnosticCode::InvalidCmdOptionValue:
            case DiagnosticCode::MissingCmdOptionValue:
            case DiagnosticCode::CmdOptionUsedTooOften:
            // Usage of the compiler
            case DiagnosticCode::MissingInputFile:
            case DiagnosticCode::MissingOutputFile:
            case DiagnosticCode::WrongFileFormat:
            case DiagnosticCode::UnknownOptimizationLevel:
            // Missing things that have to be installed for the compiler to work
            case DiagnosticCode::MissingDependency:

            // -- I/O --
            case DiagnosticCode::FileNotFound:
            case DiagnosticCode::FileIsADirectory:
            case DiagnosticCode::FileIsNotARegularFile:
            case DiagnosticCode::FailedToCreateFileStream:

            // -- Lexer --
            case DiagnosticCode::UnknownCharacter:
            case DiagnosticCode::UnknownEscapeSequence:

            // -- Parser --
            case DiagnosticCode::UnexpectedToken:
            case DiagnosticCode::InvalidReturn:
            case DiagnosticCode::InvalidMathematicalNegation:
            case DiagnosticCode::MissingEndKeyword:

            // -- Semantic analysis --
            // Module stuff
            case DiagnosticCode::UnknownModule:
            case DiagnosticCode::AmbiguousModulePath:
            case DiagnosticCode::ModuleRedefinition:
            // Type stuff
            case DiagnosticCode::UnknownType:
            case DiagnosticCode::TypeMismatch:
            case DiagnosticCode::TypeCannotBeUsedAsIdentifier:
            case DiagnosticCode::InvalidVariableType:
            case DiagnosticCode::MismatchingCastReturnType:
            // Return checking
            case DiagnosticCode::MissingReturn:
            // Symbol stuff
            case DiagnosticCode::UnknownSymbol:
            case DiagnosticCode::SymbolAlreadyExists:
            case DiagnosticCode::InvalidSymbolUsage:
            case DiagnosticCode::AmbiguousSymbolImport:
            case DiagnosticCode::SymbolNotVisible:
            case DiagnosticCode::NoMatchingSymbolCandidates:
            case DiagnosticCode::MultipleMatchingSymbolCandidates:
            // Struct stuff
            case DiagnosticCode::UnknownMember:
            case DiagnosticCode::UnknownMemberInInitializer:
            case DiagnosticCode::MemberAlreadyExists:
            case DiagnosticCode::InvalidMemberAccess:
            case DiagnosticCode::AmbiguousObjectInitializer:
            // Miscalleneous
            case DiagnosticCode::IntegerLiteralOutOfBounds:
            case DiagnosticCode::MultipleMainMethods:
            case DiagnosticCode::MismatchingOperatorParameterCount:
            case DiagnosticCode::NoOperatorImplementation:
            case DiagnosticCode::MultipleOperatorImplementations:
                return DiagnosticSeverity::Error;

            case DiagnosticCode::Unsupported:
                return DiagnosticSeverity::Unsupported;
        }
        KPL_ASSERT_UNREACHABLE("Missing format implementation for severity mapping of diagnostic code '{}'", static_cast<int>(diagnostic_code));
    }

    std::string get_diagnostic_console_highlighting(DiagnosticSeverity severity) {
        switch (severity) {
            case DiagnosticSeverity::Note:
                return ansi_codes::bold;
            case DiagnosticSeverity::Warning:
                return ansi_codes::combine(ansi_codes::bold, ansi_codes::yellow);
            case DiagnosticSeverity::Error:
                return ansi_codes::combine(ansi_codes::bold, ansi_codes::red);
            case DiagnosticSeverity::Unsupported:
                return ansi_codes::combine(ansi_codes::bold, ansi_codes::magenta);
        }
        KPL_ASSERT_UNREACHABLE("Missing styling implementation for severity '{}'", severity);
    }

}
