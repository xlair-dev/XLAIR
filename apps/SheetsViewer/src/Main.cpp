#include <Siv3D.hpp>

#include "SheetsViewer/AnalysisRenderer.hpp"
#include "SheetsViewer/ViewerSession.hpp"

#include <algorithm>

namespace {
    constexpr double SidebarWidth = 360.0;

    [[nodiscard]]
    bool IsMetadataPath(const FilePathView path) {
        const String extension = FileSystem::Extension(path).lowercased();
        return (extension == U"json" || extension == U"toml");
    }

    [[nodiscard]]
    String FormatDiagnostic(const xlair::sheets::Diagnostic& diagnostic) {
        if (diagnostic.path.isEmpty()) {
            return diagnostic.message;
        }

        String location = FileSystem::RelativePath(diagnostic.path);
        if (diagnostic.line) {
            location += U":" + Format(*diagnostic.line);
            if (diagnostic.column) {
                location += U":" + Format(*diagnostic.column);
            }
        }
        return location + U": " + diagnostic.message;
    }

    void DrawMetadata(xlair::sheets_viewer::ViewerSession& session, const Font& font) {
        const auto& metadata = session.metadata();
        if (!metadata) {
            font(U"Open metadata to begin.").draw(20, Vec2{ 24, 88 }, Palette::Lightgray);
            return;
        }

        font(metadata->title).draw(26, Vec2{ 24, 82 }, Palette::White);
        font(metadata->artist).draw(18, Vec2{ 24, 118 }, Palette::Lightgray);

        if (session.jacket()) {
            session.jacket()->resized(128).draw(24, 154);
        }

        font(U"Difficulties").draw(18, Vec2{ 24, 302 }, Palette::Skyblue);
        for (const auto& [position, difficulty] : IndexedRef(metadata->difficulties)) {
            const bool selected = session.selectedDifficultyPosition() == position;
            const String label = U"{}  {}  Lv. {}"_fmt(difficulty.index, difficulty.id, difficulty.level);
            const double y = 334.0 + static_cast<double>(position) * 36.0;
            if (SimpleGUI::Button(label, Vec2{ 24, y }, 312, true) && !selected) {
                (void)session.selectDifficulty(position);
            }

            if (selected) {
                RectF{ 18, y, 4, 28 }.draw(Palette::Skyblue);
            }
        }
    }

    void DrawDiagnostics(const xlair::sheets_viewer::ViewerSession& session, const Font& font) {
        double y = Scene::Height() - 76.0;
        const auto& diagnostics = session.diagnostics();
        for (std::size_t index = diagnostics.size(); index > 0; --index) {
            const auto& diagnostic = diagnostics[index - 1];
            const ColorF color =
                diagnostic.severity == xlair::sheets::DiagnosticSeverity::Error ? Palette::Lightcoral : Palette::Orange;
            font(FormatDiagnostic(diagnostic)).draw(15, Arg::bottomLeft = Vec2{ SidebarWidth + 40, y }, color);
            y -= 24.0;
            if (y < Scene::Height() * 0.55) {
                break;
            }
        }
    }

    void DrawTransport(xlair::sheets_viewer::ViewerSession& session, const Font& font) {
        const auto& chart = session.chart();
        const bool enabled = static_cast<bool>(chart);
        const double y = Scene::Height() - 48.0;

        if (SimpleGUI::Button(session.isPlaying() ? U"Pause" : U"Play", Vec2{ SidebarWidth + 20, y }, 72, enabled)) {
            session.togglePlayback();
        }
        if (SimpleGUI::Button(U"Stop", Vec2{ SidebarWidth + 96, y }, 64, enabled)) {
            session.stopPlayback();
        }

        const s3d::int64 duration = session.durationSamples();
        double progress = duration > 0 ? static_cast<double>(session.currentSample()) / duration : 0.0;
        const double slider_width = std::max(80.0, Scene::Width() - SidebarWidth - 370.0);
        if (SimpleGUI::Slider(progress, 0.0, 1.0, Vec2{ SidebarWidth + 176, y + 4 }, slider_width, enabled)) {
            session.seekSample(static_cast<s3d::int64>(progress * duration));
        }

        const s3d::int64 sample_rate = chart ? chart->sample_rate : 44'100;
        const String time = U"{} / {}"_fmt(
            FormatTime(SecondsF{ static_cast<double>(session.currentSample()) / sample_rate }, U"M:ss"),
            FormatTime(SecondsF{ static_cast<double>(duration) / sample_rate }, U"M:ss")
        );
        font(time).draw(15, Vec2{ Scene::Width() - 170, y + 7 }, Palette::Lightgray);
    }
}

void Main() {
    Window::SetTitle(U"SheetsViewer");
    Window::SetStyle(WindowStyle::Sizable);
    Window::Resize(1280, 720);
    Scene::SetBackground(ColorF{ 0.10, 0.11, 0.14 });

    const Font title_font{ 32 };
    const Font font{ 20 };
    xlair::sheets_viewer::ViewerSession session;
    const xlair::sheets_viewer::AnalysisRenderer renderer;
    double pixels_per_second = 420.0;

    const auto& arguments = System::GetCommandLineArgs();
    for (std::size_t index = 1; index < arguments.size(); ++index) {
        if (IsMetadataPath(arguments[index])) {
            (void)session.loadMetadata(arguments[index]);
            break;
        }
    }

    while (System::Update()) {
        session.update(Scene::DeltaTime());

        if (DragDrop::HasNewFilePaths()) {
            for (const auto& dropped : DragDrop::GetDroppedFilePaths()) {
                if (IsMetadataPath(dropped.path)) {
                    (void)session.loadMetadata(dropped.path);
                    break;
                }
            }
        }

        RectF{ 0, 0, SidebarWidth, Scene::Height() }.draw(ColorF{ 0.15, 0.16, 0.20 });
        title_font(U"SheetsViewer").draw(20, Vec2{ 24, 18 }, Palette::White);

        if (SimpleGUI::Button(U"Open metadata", Vec2{ SidebarWidth + 40, 24 }, 180)) {
            const auto path = Dialog::OpenFile({ FileFilter::JSON(), FileFilter::TOML() });
            if (path) {
                (void)session.loadMetadata(*path);
            }
        }
        if (SimpleGUI::Button(U"Reload", Vec2{ SidebarWidth + 228, 24 }, 88, static_cast<bool>(session.metadata()))) {
            (void)session.reloadMetadata();
        }

        SimpleGUI::Slider(U"Scale", pixels_per_second, 80.0, 1'000.0, Vec2{ Scene::Width() - 280, 24 }, 55, 200);

        DrawMetadata(session, font);
        if (session.chart() && session.projection()) {
            const Rect viewport{
                static_cast<s3d::int32>(SidebarWidth),
                64,
                std::max(1, Scene::Width() - static_cast<s3d::int32>(SidebarWidth)),
                std::max(1, Scene::Height() - 128),
            };
            renderer
                .draw(*session.chart(), *session.projection(), session.currentSample(), viewport, pixels_per_second);
        } else {
            font(U"No chart is loaded.").draw(22, Vec2{ SidebarWidth + 40, 88 }, Palette::Gray);
        }

        DrawDiagnostics(session, font);
        DrawTransport(session, font);
    }
}
