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

        struct PendingHispeedReference {
            TimelineId definition = 0;
            std::size_t line = 0;
        };

        struct NoteToken {
            Position position;
            s3d::uint32 kind = 0;
            s3d::uint32 width = 0;
            std::size_t column = 0;
        };

        struct ParseState {
            Document document;
            s3d::uint32 measure_base = 0;
            s3d::Optional<TimelineId> current_timeline;
            s3d::Array<PendingBPMReference> pending_bpm_references;
            s3d::Array<PendingHispeedReference> pending_hispeed_references;
            s3d::Array<Diagnostic> diagnostics;
        };

        void AddError(
            ParseState& state,
            s3d::String message,
            const s3d::FilePath& path,
            const std::size_t line,
            const s3d::Optional<std::size_t> column = s3d::none
        ) {
            state.diagnostics.push_back(
                {
                    .severity = DiagnosticSeverity::Error,
                    .message = std::move(message),
                    .path = path,
                    .line = line,
                    .column = column,
                }
            );
        }

        void AppendDiagnostics(ParseState& state, const s3d::Array<Diagnostic>& diagnostics) {
            for (const auto& diagnostic : diagnostics) {
                state.diagnostics.push_back(diagnostic);
            }
        }

        [[nodiscard]]
        s3d::Optional<std::size_t> ArgumentColumn(const CommandLine& command) {
            if (command.argument_column == 0) {
                return s3d::none;
            }

            return command.argument_column;
        }

        [[nodiscard]]
        s3d::Optional<s3d::uint32> ParseDecimalUInt32(const s3d::StringView value) {
            if (value.isEmpty() || !s3d::String{ value }.all(s3d::IsDigit)) {
                return s3d::none;
            }

            return s3d::ParseIntOpt<s3d::uint32>(value, s3d::Arg::radix = 10);
        }

        [[nodiscard]]
        s3d::Optional<s3d::uint32> EffectiveMeasure(
            ParseState& state,
            const s3d::uint32 measure,
            const s3d::FilePath& path,
            const std::size_t line
        ) {
            if (measure > (std::numeric_limits<s3d::uint32>::max() - state.measure_base)) {
                AddError(state, U"Measure number exceeds the uint32 range after applying #MEASUREBS.", path, line);
                return s3d::none;
            }

            return state.measure_base + measure;
        }

        [[nodiscard]]
        s3d::Optional<s3d::Array<NoteToken>>
        ParseNoteTokens(ParseState& state, const DataLine& data, const s3d::FilePath& path, const std::size_t line) {
            const auto measure = EffectiveMeasure(state, data.measure, path, line);
            if (!measure) {
                return s3d::none;
            }

            if ((data.data.size() % 2) != 0) {
                AddError(
                    state,
                    U"Note data must contain two characters per subdivision.",
                    path,
                    line,
                    data.data_column
                );
                return s3d::none;
            }

            const std::size_t subdivision_count = data.data.size() / 2;
            s3d::Array<NoteToken> tokens;
            for (std::size_t index = 0; index < subdivision_count; ++index) {
                const std::size_t column = data.data_column + (index * 2);
                const auto kind = ParseBase36(data.data.substr(index * 2, 1), line, column, path);
                const auto width = ParseBase36(data.data.substr((index * 2) + 1, 1), line, column + 1, path);
                if (!kind || !width) {
                    if (!kind) {
                        AppendDiagnostics(state, kind.diagnostics);
                    }
                    if (!width) {
                        AppendDiagnostics(state, width.diagnostics);
                    }
                    continue;
                }

                if (*kind == 0) {
                    continue;
                }

                if (*width == 0) {
                    AddError(state, U"Placed notes require a width between 1 and Z.", path, line, column + 1);
                    continue;
                }

                tokens.push_back({
                    .position = {
                        .measure = *measure,
                        .numerator = static_cast<s3d::uint32>(index),
                        .denominator = static_cast<s3d::uint32>(subdivision_count),
                    },
                    .kind = *kind,
                    .width = *width,
                    .column = column,
                });
            }

            return tokens;
        }

        [[nodiscard]]
        s3d::Optional<s3d::uint8>
        ParseLane(ParseState& state, const DataLine& data, const s3d::FilePath& path, const std::size_t line) {
            const auto lane = ParseBase36(data.code.substr(1, 1), line, 6, path);
            if (!lane) {
                AppendDiagnostics(state, lane.diagnostics);
                return s3d::none;
            }

            return static_cast<s3d::uint8>(*lane);
        }

        [[nodiscard]]
        s3d::Optional<ChannelId>
        ParseChannel(ParseState& state, const DataLine& data, const s3d::FilePath& path, const std::size_t line) {
            const auto channel = ParseBase36(data.code.substr(2, 1), line, 7, path);
            if (!channel) {
                AppendDiagnostics(state, channel.diagnostics);
                return s3d::none;
            }

            return *channel;
        }

        [[nodiscard]]
        s3d::Optional<SliderNoteKind> ToSliderNoteKind(const s3d::uint32 kind) {
            switch (kind) {
                case 1:
                    return SliderNoteKind::Tap1;
                case 2:
                    return SliderNoteKind::Tap2;
                case 3:
                    return SliderNoteKind::Tap3;
                case 4:
                    return SliderNoteKind::Tap4;
                case 5:
                    return SliderNoteKind::Tap5;
                case 6:
                    return SliderNoteKind::Tap6;
                default:
                    return s3d::none;
            }
        }

        [[nodiscard]]
        s3d::Optional<DirectionalKind> ToDirectionalKind(const s3d::uint32 kind) {
            switch (kind) {
                case 1:
                    return DirectionalKind::Up;
                case 2:
                    return DirectionalKind::Down;
                case 3:
                    return DirectionalKind::LeftUp;
                case 4:
                    return DirectionalKind::RightUp;
                case 5:
                    return DirectionalKind::LeftDown;
                case 6:
                    return DirectionalKind::RightDown;
                default:
                    return s3d::none;
            }
        }

        [[nodiscard]]
        s3d::Optional<SliderHoldPointKind> ToSliderHoldPointKind(const s3d::uint32 kind) {
            switch (kind) {
                case 1:
                    return SliderHoldPointKind::Start;
                case 2:
                    return SliderHoldPointKind::End;
                case 3:
                    return SliderHoldPointKind::Visible;
                case 4:
                    return SliderHoldPointKind::Control;
                case 5:
                    return SliderHoldPointKind::Invisible;
                default:
                    return s3d::none;
            }
        }

        [[nodiscard]]
        s3d::Optional<SideLongPointKind> ToSideLongPointKind(const s3d::uint32 kind) {
            switch (kind) {
                case 1:
                    return SideLongPointKind::Start;
                case 2:
                    return SideLongPointKind::End;
                case 3:
                    return SideLongPointKind::Relay;
                default:
                    return s3d::none;
            }
        }

        void InterpretRequest(
            ParseState& state,
            const CommandLine& command,
            const s3d::FilePath& path,
            const std::size_t line
        ) {
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

        void InterpretBPMDefinition(
            ParseState& state,
            const CommandLine& command,
            const s3d::FilePath& path,
            const std::size_t line
        ) {
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

        void InterpretHispeedDefinition(
            ParseState& state,
            const CommandLine& command,
            const s3d::FilePath& path,
            const std::size_t line
        ) {
            if (command.key.size() != 5) {
                AddError(state, U"Hispeed definition names must use the form #TILzz.", path, line);
                return;
            }

            const auto definition_id = ParseBase36(command.key.substr(3), line, 5, path);
            if (!definition_id) {
                AppendDiagnostics(state, definition_id.diagnostics);
                return;
            }

            if (command.argument.size() < 2 || command.argument.front() != U'"' || command.argument.back() != U'"') {
                AddError(
                    state,
                    U"#TILzz values must be enclosed in double quotes.",
                    path,
                    line,
                    ArgumentColumn(command)
                );
                return;
            }

            const s3d::String value = command.argument.substr(1, command.argument.size() - 2);
            if (value.trimmed().isEmpty()) {
                state.document.hispeed_definitions[*definition_id] = {};
                return;
            }

            const auto entries = value.split(U',');

            HispeedDefinition definition;
            for (const auto& raw_entry : entries) {
                const s3d::String entry = raw_entry.trimmed();
                const std::size_t speed_separator = entry.indexOf(U':');
                const std::size_t tick_separator = entry.indexOf(U'\'');
                if (speed_separator == s3d::String::npos || tick_separator == s3d::String::npos ||
                    tick_separator >= speed_separator) {
                    AddError(state, U"Hispeed changes must use the form measure'tick:speed.", path, line);
                    return;
                }

                const auto measure = ParseDecimalUInt32(entry.substr(0, tick_separator).trimmed());
                const auto tick = ParseDecimalUInt32(
                    entry.substr(tick_separator + 1, speed_separator - tick_separator - 1).trimmed()
                );
                const auto multiplier = s3d::ParseFloatOpt<double>(entry.substr(speed_separator + 1).trimmed());
                if (!measure || !tick || !multiplier || !std::isfinite(*multiplier)) {
                    AddError(
                        state,
                        U"Hispeed changes require a non-negative measure and tick and a finite speed.",
                        path,
                        line
                    );
                    return;
                }

                definition.changes.push_back({
                    .position = {
                        .measure = *measure,
                        .tick = *tick,
                    },
                    .multiplier = *multiplier,
                });
            }

            state.document.hispeed_definitions[*definition_id] = std::move(definition);
        }

        void InterpretHispeedSelection(
            ParseState& state,
            const CommandLine& command,
            const s3d::FilePath& path,
            const std::size_t line
        ) {
            if (command.argument.size() != 2) {
                AddError(
                    state,
                    U"#HISPEED requires a two-character Base36 definition ID.",
                    path,
                    line,
                    ArgumentColumn(command)
                );
                return;
            }

            const auto definition = ParseBase36(command.argument, line, command.argument_column, path);
            if (!definition) {
                AppendDiagnostics(state, definition.diagnostics);
                return;
            }

            state.current_timeline = *definition;
            state.pending_hispeed_references.push_back(
                {
                    .definition = *definition,
                    .line = line,
                }
            );
        }

        void InterpretCommand(
            ParseState& state,
            const CommandLine& command,
            const s3d::FilePath& path,
            const std::size_t line
        ) {
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

            if (command.key == U"HISPEED") {
                InterpretHispeedSelection(state, command, path, line);
                return;
            }

            if (command.key == U"NOSPEED") {
                if (!command.argument.isEmpty()) {
                    AddError(state, U"#NOSPEED does not accept an argument.", path, line, ArgumentColumn(command));
                    return;
                }

                state.current_timeline = s3d::none;
                return;
            }

            if (command.key.starts_with(U"BPM")) {
                InterpretBPMDefinition(state, command, path, line);
                return;
            }

            if (command.key.starts_with(U"TIL")) {
                InterpretHispeedDefinition(state, command, path, line);
            }
        }

        void InterpretBeatsPerMeasure(
            ParseState& state,
            const DataLine& data,
            const s3d::FilePath& path,
            const std::size_t line
        ) {
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

        void InterpretBPMChanges(
            ParseState& state,
            const DataLine& data,
            const s3d::FilePath& path,
            const std::size_t line
        ) {
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
                state.pending_bpm_references.push_back(
                    {
                        .definition = *definition,
                        .line = line,
                    }
                );
            }
        }

        void InterpretSliderNotes(
            ParseState& state,
            const DataLine& data,
            const s3d::FilePath& path,
            const std::size_t line
        ) {
            if (data.code.size() != 2) {
                AddError(state, U"Short note headers must use the form #mmm1x.", path, line, 5);
                return;
            }

            const auto lane = ParseLane(state, data, path, line);
            const auto tokens = ParseNoteTokens(state, data, path, line);
            if (!lane || !tokens) {
                return;
            }

            for (const auto& token : *tokens) {
                const auto kind = ToSliderNoteKind(token.kind);
                if (!kind) {
                    AddError(state, U"Short note kinds must be between 1 and 6.", path, line, token.column);
                    continue;
                }

                state.document.slider_notes.push_back({
                    .kind = *kind,
                    .position = token.position,
                    .lane = {
                        .start = *lane,
                        .width = static_cast<s3d::uint8>(token.width),
                    },
                    .timeline = state.current_timeline,
                });
            }
        }

        void InterpretDirectionalNotes(
            ParseState& state,
            const DataLine& data,
            const s3d::FilePath& path,
            const std::size_t line
        ) {
            if (data.code.size() != 2) {
                AddError(state, U"Directional note headers must use the form #mmm5x.", path, line, 5);
                return;
            }

            const auto lane = ParseLane(state, data, path, line);
            const auto tokens = ParseNoteTokens(state, data, path, line);
            if (!lane || !tokens) {
                return;
            }

            for (const auto& token : *tokens) {
                const auto kind = ToDirectionalKind(token.kind);
                if (!kind) {
                    AddError(state, U"Directional note kinds must be between 1 and 6.", path, line, token.column);
                    continue;
                }

                state.document.directional_notes.push_back({
                    .kind = *kind,
                    .position = token.position,
                    .lane = {
                        .start = *lane,
                        .width = static_cast<s3d::uint8>(token.width),
                    },
                    .timeline = state.current_timeline,
                });
            }
        }

        void InterpretSliderHoldPoints(
            ParseState& state,
            const DataLine& data,
            const s3d::FilePath& path,
            const std::size_t line
        ) {
            if (data.code.size() != 3) {
                AddError(state, U"Slider hold headers must use the form #mmm3xy.", path, line, 5);
                return;
            }

            const auto lane = ParseLane(state, data, path, line);
            const auto channel = ParseChannel(state, data, path, line);
            const auto tokens = ParseNoteTokens(state, data, path, line);
            if (!lane || !channel || !tokens) {
                return;
            }

            for (const auto& token : *tokens) {
                const auto kind = ToSliderHoldPointKind(token.kind);
                if (!kind) {
                    AddError(state, U"Slider hold point kinds must be between 1 and 5.", path, line, token.column);
                    continue;
                }

                state.document.slider_hold_points.push_back({
                    .kind = *kind,
                    .position = token.position,
                    .lane = {
                        .start = *lane,
                        .width = static_cast<s3d::uint8>(token.width),
                    },
                    .channel = *channel,
                    .timeline = state.current_timeline,
                });
            }
        }

        void InterpretSideLongPoints(
            ParseState& state,
            const DataLine& data,
            const s3d::FilePath& path,
            const std::size_t line
        ) {
            if (data.code.size() != 3) {
                AddError(state, U"SideLong headers must use the form #mmm2xy.", path, line, 5);
                return;
            }

            const auto lane = ParseLane(state, data, path, line);
            const auto channel = ParseChannel(state, data, path, line);
            const auto tokens = ParseNoteTokens(state, data, path, line);
            if (!lane || !channel || !tokens) {
                return;
            }

            for (const auto& token : *tokens) {
                const auto kind = ToSideLongPointKind(token.kind);
                if (!kind) {
                    AddError(
                        state,
                        U"SideLong point kinds must be 1 (Start), 2 (End), or 3 (Relay).",
                        path,
                        line,
                        token.column
                    );
                    continue;
                }

                state.document.side_long_points.push_back({
                    .kind = *kind,
                    .position = token.position,
                    .lane = {
                        .start = *lane,
                        .width = static_cast<s3d::uint8>(token.width),
                    },
                    .channel = *channel,
                    .timeline = state.current_timeline,
                });
            }
        }

        void InterpretData(ParseState& state, const DataLine& data, const s3d::FilePath& path, const std::size_t line) {
            if (data.code == U"02") {
                InterpretBeatsPerMeasure(state, data, path, line);
            } else if (data.code == U"08") {
                InterpretBPMChanges(state, data, path, line);
            } else if (data.code.starts_with(U'1')) {
                InterpretSliderNotes(state, data, path, line);
            } else if (data.code.starts_with(U'2')) {
                InterpretSideLongPoints(state, data, path, line);
            } else if (data.code.starts_with(U'3')) {
                InterpretSliderHoldPoints(state, data, path, line);
            } else if (data.code.starts_with(U'5')) {
                InterpretDirectionalNotes(state, data, path, line);
            }
        }

        void
        InterpretLine(ParseState& state, const ParsedLine& parsed, const s3d::FilePath& path, const std::size_t line) {
            if (const auto* command = std::get_if<CommandLine>(&parsed)) {
                InterpretCommand(state, *command, path, line);
            } else if (const auto* data = std::get_if<DataLine>(&parsed)) {
                InterpretData(state, *data, path, line);
            }
        }

        [[nodiscard]]
        Result<Document> ParseLines(const s3d::Array<s3d::String>& lines, const s3d::FilePath& path) {
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

            for (const auto& reference : state.pending_hispeed_references) {
                if (!state.document.hispeed_definitions.contains(reference.definition)) {
                    AddError(state, U"#HISPEED references an undefined hispeed definition.", path, reference.line);
                }
            }

            if (state.diagnostics.isEmpty()) {
                return Result<Document>{ std::move(state.document) };
            }

            Result<Document> result;
            result.diagnostics = std::move(state.diagnostics);
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
