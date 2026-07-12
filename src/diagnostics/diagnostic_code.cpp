// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "diagnostics/diagnostic_code.hpp"
#include "diagnostics/severity.hpp"
#include "log.hpp"

namespace kepler::diagnostics {

    Severity get_severity(DiagnosticCode diagnostic_code) {
        switch (diagnostic_code) {
            case DiagnosticCode::MultilineCommentNotClosed:
                return Severity::Warning;

            case DiagnosticCode::NoInputFile:
            case DiagnosticCode::NoOutputFile:
            case DiagnosticCode::TooManyInputFiles:
            case DiagnosticCode::TooManyOutputFiles:
            case DiagnosticCode::CxxoptsException:
            case DiagnosticCode::UnknownCharacter:
            case DiagnosticCode::UnknownEscapeSequence:
            case DiagnosticCode::UnexpectedToken:
            case DiagnosticCode::InvalidCast:
            case DiagnosticCode::InvalidReturnExpression:
            case DiagnosticCode::InvalidVariableType:
            case DiagnosticCode::InvalidLoopVariableType:
            case DiagnosticCode::InvalidMathematicalNegation:
            case DiagnosticCode::MissingEndKeyword:
            case DiagnosticCode::UsingStatementAsExpression:
            case DiagnosticCode::UndefinedSymbol:
            case DiagnosticCode::SymbolAlreadyExists:
                return Severity::Error;

            case DiagnosticCode::Unsupported:
                return Severity::Unsupported;

            default:
                log::warning("No severity mapping for diagnostic code '{}', defaulting to error", static_cast<int>(diagnostic_code));
                return Severity::Error;
        }
    }

}
