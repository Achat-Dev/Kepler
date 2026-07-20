// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2026 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "diagnostics/diagnostic.hpp"
#include "log.hpp"

namespace kepler {

    DiagnosticSeverity get_diagnostic_severity(DiagnosticCode diagnostic_code) {
        switch (diagnostic_code) {
            case DiagnosticCode::MultilineCommentNotClosed:
                return DiagnosticSeverity::Warning;

            case DiagnosticCode::NoInputFile:
            case DiagnosticCode::NoOutputFile:
            case DiagnosticCode::TooManyInputFiles:
            case DiagnosticCode::TooManyOutputFiles:
            case DiagnosticCode::CxxoptsException:
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
                return DiagnosticSeverity::Error;

            case DiagnosticCode::Unsupported:
                return DiagnosticSeverity::Unsupported;

            default:
                log::warning("No severity mapping for diagnostic code '{}', defaulting to error", static_cast<int>(diagnostic_code));
                return DiagnosticSeverity::Error;
        }
    }

}
