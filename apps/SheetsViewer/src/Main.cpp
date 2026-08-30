#include <Siv3D.hpp>
#include <SheetsAnalyzer.hpp>

void Main() {
    Scene::SetBackground(ColorF{ 0.16, 0.18, 0.22 });

    const Font titleFont{ 32 };
    const Font font{ 24 };
    const xlair::sheets::Metadata metadata;
    const xlair::sheets::ChartLoadOptions chartLoadOptions;

    while (System::Update()) {
        titleFont(U"SheetsViewer").draw(40, 40, Palette::White);
        font(U"Default title: {}"_fmt(metadata.title)).draw(40, 96, Palette::Skyblue);
        font(U"Sample rate: {} Hz"_fmt(chartLoadOptions.sample_rate)).draw(40, 136, Palette::Lightgray);
    }
}
