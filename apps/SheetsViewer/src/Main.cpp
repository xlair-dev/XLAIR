#include <Siv3D.hpp>

#include "SheetsViewer/AnalysisRenderer.hpp"
#include "SheetsViewer/SideMenu.hpp"
#include "SheetsViewer/ViewerSession.hpp"

#include <Siv3D/SimpleGUI.hpp>
#include <algorithm>

namespace {
    constexpr s3d::int32 TransportHeight = 35;
    constexpr s3d::int32 ButtonWidth = 45;

    constexpr MenuBarItemIndex OpenMetadataItem{ 0, 0 };
    constexpr MenuBarItemIndex ReloadMetadataItem{ 0, 1 };
    constexpr MenuBarItemIndex ExitItem{ 0, 2 };

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

    void DrawDiagnostics(const xlair::sheets_viewer::ViewerSession& session, const Font& font) {
        double y = Scene::Height() - TransportHeight - 12.0;
        const auto& diagnostics = session.diagnostics();
        for (std::size_t index = diagnostics.size(); index > 0; --index) {
            const auto& diagnostic = diagnostics[index - 1];
            const ColorF color =
                diagnostic.severity == xlair::sheets::DiagnosticSeverity::Error ? Palette::Lightcoral : Palette::Orange;
            font(FormatDiagnostic(diagnostic))
                .draw(15, Arg::bottomLeft = Vec2{ xlair::sheets_viewer::SideMenu::Width + 20, y }, color);
            y -= 24.0;
            if (y < Scene::Height() * 0.55) {
                break;
            }
        }
    }

    void DrawTransport(xlair::sheets_viewer::ViewerSession& session, double& pixels_per_second) {
        const auto& chart = session.chart();
        const bool enabled = static_cast<bool>(chart);
        const double y = Scene::Height() - TransportHeight;
        RectF{
            xlair::sheets_viewer::SideMenu::Width,
            y,
            std::max(0, Scene::Width() - xlair::sheets_viewer::SideMenu::Width),
            TransportHeight,
        }
            .draw(ColorF{ 0.12, 0.13, 0.16 });

        const String play_icon = session.isPlaying() ? U"\U000F03E4" : U"\U000F040A";
        if (SimpleGUI::Button(play_icon, Vec2{ xlair::sheets_viewer::SideMenu::Width, y }, ButtonWidth, enabled)) {
            session.togglePlayback();
        }
        if (SimpleGUI::Button(
                U"\U000F04DB",
                Vec2{ xlair::sheets_viewer::SideMenu::Width + ButtonWidth, y },
                ButtonWidth,
                enabled
            )) {
            session.stopPlayback();
        }

        const s3d::int64 duration = session.durationSamples();
        double progress = duration > 0 ? static_cast<double>(session.currentSample()) / duration : 0.0;
        const s3d::int64 sample_rate = chart ? chart->sample_rate : 44'100;
        const String time = U"{} / {}"_fmt(
            FormatTime(SecondsF{ static_cast<double>(session.currentSample()) / sample_rate }, U"M:ss"),
            FormatTime(SecondsF{ static_cast<double>(duration) / sample_rate }, U"M:ss")
        );
        const double seek_x = xlair::sheets_viewer::SideMenu::Width + ButtonWidth * 2;
        const bool show_scale = (Scene::Width() >= 900);
        const double scale_width = show_scale ? 195.0 : 0.0;
        const double seek_width = std::max(80.0, Scene::Width() - seek_x - scale_width - 100.0);
        if (SimpleGUI::Slider(time, progress, 0.0, 1.0, Vec2{ seek_x, y }, 100.0, seek_width, enabled)) {
            session.seekSample(static_cast<s3d::int64>(progress * duration));
        }

        if (show_scale) {
            SimpleGUI::Slider(
                U"Scale",
                pixels_per_second,
                80.0,
                1'000.0,
                Vec2{ Scene::Width() - scale_width, y },
                55,
                140
            );
        }
    }
}

void Main() {
    Window::SetTitle(U"SheetsViewer");
    Window::SetStyle(WindowStyle::Sizable);
    Window::Resize(1280, 720);
    Scene::SetBackground(ColorF{ 0.10, 0.11, 0.14 });

    const Font font{ 20 };
    xlair::sheets_viewer::ViewerSession session;
    const xlair::sheets_viewer::AnalysisRenderer renderer;
    const xlair::sheets_viewer::SideMenu side_menu;
    double pixels_per_second = 420.0;

    const Array<std::pair<String, Array<String>>> menu_items{
        {
            U"File",
            {
                U"\U000F0214 Open Metadata...",
                U"\U000F0453 Reload Metadata",
                U"\U000F05AD Exit",
            },
        },
    };
    SimpleMenuBar menu_bar{ menu_items };

    const auto& arguments = System::GetCommandLineArgs();
    for (std::size_t index = 1; index < arguments.size(); ++index) {
        if (IsMetadataPath(arguments[index])) {
            (void)session.loadMetadata(arguments[index]);
            break;
        }
    }

    while (System::Update()) {
        session.update(Scene::DeltaTime());

        menu_bar.setItemEnabled(ReloadMetadataItem, static_cast<bool>(session.metadata()));
        if (const auto item = menu_bar.update()) {
            if (*item == OpenMetadataItem) {
                const auto path = Dialog::OpenFile({ FileFilter::JSON(), FileFilter::TOML() });
                if (path) {
                    (void)session.loadMetadata(*path);
                }
            } else if (*item == ReloadMetadataItem) {
                (void)session.reloadMetadata();
            } else if (*item == ExitItem) {
                System::Exit();
            }
        }

        if (DragDrop::HasNewFilePaths()) {
            for (const auto& dropped : DragDrop::GetDroppedFilePaths()) {
                if (IsMetadataPath(dropped.path)) {
                    (void)session.loadMetadata(dropped.path);
                    break;
                }
            }
        }

        const s3d::int32 content_height = std::max(1, Scene::Height() - SimpleMenuBar::MenuBarHeight);
        if (session.chart() && session.projection()) {
            const Rect viewport{
                xlair::sheets_viewer::SideMenu::Width,
                SimpleMenuBar::MenuBarHeight,
                std::max(1, Scene::Width() - xlair::sheets_viewer::SideMenu::Width),
                content_height,
            };
            renderer
                .draw(*session.chart(), *session.projection(), session.currentSample(), viewport, pixels_per_second);
        } else {
            Rect{
                xlair::sheets_viewer::SideMenu::Width,
                SimpleMenuBar::MenuBarHeight,
                std::max(1, Scene::Width() - xlair::sheets_viewer::SideMenu::Width),
                content_height,
            }
                .draw(ColorF{ 0.08, 0.09, 0.12 });
            font(U"No chart is loaded.")
                .draw(
                    22,
                    Vec2{
                        xlair::sheets_viewer::SideMenu::Width + 24,
                        SimpleMenuBar::MenuBarHeight + 24,
                    },
                    Palette::Gray
                );
        }

        side_menu.draw(
            session,
            Rect{
                0,
                SimpleMenuBar::MenuBarHeight,
                xlair::sheets_viewer::SideMenu::Width,
                std::max(1, Scene::Height() - SimpleMenuBar::MenuBarHeight),
            }
        );
        DrawDiagnostics(session, font);
        DrawTransport(session, pixels_per_second);
        menu_bar.draw();
    }
}
