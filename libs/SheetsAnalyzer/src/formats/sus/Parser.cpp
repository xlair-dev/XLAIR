#include "Parser.hpp"

#include "Syntax.hpp"

#include <Siv3D/Char.hpp>
#include <Siv3D/ParseFloat.hpp>
#include <Siv3D/ParseInt.hpp>
#include <Siv3D/TextReader.hpp>

#include <cmath>
#include <limits>
#include <utility>
#include <variant>

namespace xlair::sheets::formats::sus {
    namespace {
        struct PendingBPMReference {
            DefinitionId definition = 0;
            std::size_t line = 0;
        };

        struct ParseState {
            Document document;
            s3d::uint32 measure_base = 0;
            s3d::Array<PendingBPMReference> pending_bpm_references;
            s3d::Array<Diagnostic> diagnostics;
        };

        void AddError(ParseState& state, s3d::String message, const s3d::FilePath& path, const std::size_t line) {
            state.diagnostics.push_back({
                .severity = DiagnosticSeverity::Error,
                .message = std::move(message),
                .path = path,
                .line = line,
                .column = s3d::none,
            });
        }

        void AppendDiagnostics(ParseState& state, const s3d::Array<Diagnostic>& diagnostics) {
            for (const auto& diagnostic : diagnostics) {
                state.diagnostics.push_back(diagnostic);
            }
        }

        [[nodiscard]] s3d::Optional<s3d::uint32> ParseDecimalUInt32(const s3d::StringView value) {
            if (value.isEmpty() || !s3d::String{ value }.all(s3d::IsDigit)) {
                return s3d::none;
            }

            return s3d::ParseIntOpt<s3d::uint32>(value, s3d::Arg::radix = 10);
        }

        [[nodiscard]] s3d::Optional<s3d::uint32> EffectiveMeasure(ParseState& state, const s3d::uint32 measure,
                                                                  const s3d::FilePath& path, const std::size_t line) {
            if (measure > (std::numeric_limits<s3d::uint32>::max() - state.measure_base)) {
                AddError(state, U"Measure number exceeds the uint32 range after applying #MEASUREBS.", path, line);
                return s3d::none;
            }

            return state.measure_base + measure;
        }

        void InterpretRequest(ParseState& state, const CommandLine& command, const s3d::FilePath& path,
                              const std::size_t line) {
            if (command.argument.size() < 2 || command.argument.front() != U'"' || command.argument.back() != U'"') {
                AddError(state, U"#REQUEST arguments must be enclosed in double quotes.", path, line);
                return;
            }

            const s3d::String request = command.argument.substr(1, command.argument.size() - 2).trimmed();
            const std::size_t separator = request.indexOfAny(U" \t");
            const s3d::String name = request.substr(0, separator).lowercased();

            if (separator == s3d::String::npos) {
                if (name == U"ticks_per_beat" || name == U"enable_priority") {
                    AddError(state, U"#REQUEST " + name + U" requires a value.", path, line);
                }
                return;
            }

            const s3d::String value = request.substr(separator + 1).trimmed();
            if (name == U"ticks_per_beat") {
                const auto ticks = ParseDecimalUInt32(value);
                if (!ticks || *ticks == 0) {
                    AddError(state, U"#REQUEST ticks_per_beat requires a positive integer value.", path, line);
                    return;
                }

                state.document.ticks_per_beat = *ticks;
                return;
            }

            if (name == U"enable_priority") {
                const s3d::String normalized = value.lowercased();
                if (normalized == U"true") {
                    state.document.priority_enabled = true;
                } else if (normalized == U"false") {
                    state.document.priority_enabled = false;
                } else {
                    AddError(state, U"#REQUEST enable_priority requires true or false.", path, line);
                }
            }
        }

        void InterpretBPMDefinition(ParseState& state, const CommandLine& command, const s3d::FilePath& path,
                                    const std::size_t line) {
            if (command.key.size() != 5) {
                AddError(state, U"BPM definition names must use the form #BPMzz.", path, line);
                return;
            }

            const auto definition = ParseBase36(command.key.substr(3), line, 5, path);
            if (!definition) {
                AppendDiagnostics(state, definition.diagnostics);
                return;
            }

            if (*definition == 0) {
                AddError(state, U"BPM definition ID 00 is reserved for an empty data slot.", path, line);
                return;
            }

            const auto bpm = s3d::ParseFloatOpt<double>(command.argument);
            if (!bpm || !std::isfinite(*bpm) || *bpm <= 0.0) {
                AddError(state, U"BPM definitions require a positive finite number.", path, line);
                return;
            }

            state.document.bpm_definitions[*definition] = *bpm;
        }

        void InterpretCommand(ParseState& state, const CommandLine& command, const s3d::FilePath& path,
                              const std::size_t line) {
            if (command.key == U"REQUEST") {
                InterpretRequest(state, command, path, line);
                return;
            }

            if (command.key == U"MEASUREBS") {
                const auto measure_base = ParseDecimalUInt32(command.argument);
                if (!measure_base) {
                    AddError(state, U"#MEASUREBS requires a non-negative decimal integer.", path, line);
                    return;
                }

                state.measure_base = *measure_base;
                return;
            }

            if (command.key.starts_with(U"BPM")) {
                InterpretBPMDefinition(state, command, path, line);
            }
        }

        void InterpretBeatsPerMeasure(ParseState& state, const DataLine& data, const s3d::FilePath& path,
                                      const std::size_t line) {
            const auto measure = EffectiveMeasure(state, data.measure, path, line);
            if (!measure) {
                return;
            }

            const auto beats = s3d::ParseFloatOpt<double>(data.data);
            if (!beats || !std::isfinite(*beats) || *beats <= 0.0) {
                AddError(state, U"#mmm02 requires a positive finite number of beats.", path, line);
                return;
            }

            state.document.beats_per_measure[*measure] = *beats;
        }

        void InterpretBPMChanges(ParseState& state, const DataLine& data, const s3d::FilePath& path,
                                 const std::size_t line) {
            const auto measure = EffectiveMeasure(state, data.measure, path, line);
            if (!measure) {
                return;
            }

            if ((data.data.size() % 2) != 0) {
                AddError(state, U"#mmm08 data must contain two characters per subdivision.", path, line);
                return;
            }

            const std::size_t subdivision_count = data.data.size() / 2;
            for (std::size_t index = 0; index < subdivision_count; ++index) {
                const auto definition =
                    ParseBase36(data.data.substr(index * 2, 2), line, data.data_column + (index * 2), path);
                if (!definition) {
                    AppendDiagnostics(state, definition.diagnostics);
                    continue;
                }

                if (*definition == 0) {
                    continue;
                }

                state.document.bpm_changes.push_back({
                    .position = {
                        .measure = *measure,
                        .numerator = static_cast<s3d::uint32>(index),
                        .denominator = static_cast<s3d::uint32>(subdivision_count),
                    },
                    .definition = *definition,
                });
                state.pending_bpm_references.push_back({
                    .definition = *definition,
                    .line = line,
                });
            }
        }

        void InterpretData(ParseState& state, const DataLine& data, const s3d::FilePath& path, const std::size_t line) {
            if (data.code == U"02") {
                InterpretBeatsPerMeasure(state, data, path, line);
            } else if (data.code == U"08") {
                InterpretBPMChanges(state, data, path, line);
            }
        }

        void InterpretLine(ParseState& state, const ParsedLine& parsed, const s3d::FilePath& path,
                           const std::size_t line) {
            if (const auto* command = std::get_if<CommandLine>(&parsed)) {
                InterpretCommand(state, *command, path, line);
            } else if (const auto* data = std::get_if<DataLine>(&parsed)) {
                InterpretData(state, *data, path, line);
            }
        }

        [[nodiscard]] Result<Document> ParseLines(const s3d::Array<s3d::String>& lines, const s3d::FilePath& path) {
            ParseState state;

            for (std::size_t index = 0; index < lines.size(); ++index) {
                const std::size_t line_number = index + 1;
                const auto parsed = ParseLine(lines[index], line_number, path);
                if (!parsed) {
                    AppendDiagnostics(state, parsed.diagnostics);
                    continue;
                }

                InterpretLine(state, *parsed, path, line_number);
            }

            for (const auto& reference : state.pending_bpm_references) {
                if (!state.document.bpm_definitions.contains(reference.definition)) {
                    AddError(state, U"BPM change references an undefined BPM ID.", path, reference.line);
                }
            }

            Result<Document> result;
            result.diagnostics = std::move(state.diagnostics);
            if (result.diagnostics.isEmpty()) {
                result.value = std::move(state.document);
            }
            return result;
        }
    }

    Result<Document> Parse(const s3d::FilePath& path) {
        s3d::TextReader reader{ path };
        if (!reader) {
            return Result<Document>::makeError(U"Failed to open the SUS file.", path);
        }

        if (reader.encoding() != s3d::TextEncoding::UTF8_NO_BOM &&
            reader.encoding() != s3d::TextEncoding::UTF8_WITH_BOM) {
            return Result<Document>::makeError(U"SUS files must use UTF-8 encoding.", path);
        }

        s3d::Array<s3d::String> lines;
        if (!reader.readLines(lines)) {
            return Result<Document>::makeError(U"Failed to read the SUS file.", path);
        }

        return ParseLines(lines, path);
    }

    Result<Document> ParseText(const s3d::StringView text, const s3d::FilePath& path) {
        return ParseLines(s3d::String{ text }.split_lines(), path);
    }
}
