// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2026 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "diagnostics/diagnostic.hpp"
#include <cassert>
#include <utility>

namespace kepler {

    DiagnosticSeverity get_diagnostic_severity(DiagnosticCode diagnostic_code) {
        switch (diagnostic_code) {
            case DiagnosticCode::HelpRequested:
                return DiagnosticSeverity::Note;

            case DiagnosticCode::MultilineCommentNotClosed:
            case DiagnosticCode::RedundantCast:
                return DiagnosticSeverity::Warning;

            case DiagnosticCode::NoInputFile:
            case DiagnosticCode::NoOutputFile:
            case DiagnosticCode::TooManyInputFiles:
            case DiagnosticCode::TooManyOutputFiles:
            case DiagnosticCode::CxxoptsException:
            case DiagnosticCode::FileNotFound:
            case DiagnosticCode::FileIsADirectory:
            case DiagnosticCode::NotARegularFile:
            case DiagnosticCode::FailedToCreateFileStream:
            case DiagnosticCode::UnknownCharacter:
            case DiagnosticCode::UnknownEscapeSequence:
            case DiagnosticCode::UnexpectedToken:
            case DiagnosticCode::InvalidCast:
            case DiagnosticCode::InvalidReturn:
            case DiagnosticCode::InvalidVariableType:
            case DiagnosticCode::InvalidLoopVariableType:
            case DiagnosticCode::InvalidMathematicalNegation:
            case DiagnosticCode::MissingEndKeyword:
            case DiagnosticCode::UsingStatementAsExpression:
            case DiagnosticCode::UndefinedSymbol:
            case DiagnosticCode::SymbolAlreadyExists:
            case DiagnosticCode::UnknownType:
            case DiagnosticCode::InvalidFunctionCall:
            case DiagnosticCode::TypeMismatch:
            case DiagnosticCode::UnsupportedMathematicalOperation:
                return DiagnosticSeverity::Error;

            case DiagnosticCode::Unsupported:
                return DiagnosticSeverity::Unsupported;
        }

        assert(false && "Missing format implementation for severity mapping of diagnostic code");
        std::unreachable();
    }

}
