# include <Siv3D.hpp>
# include <SheetsAnalyzer/SheetsAnalyzer.hpp>

void Main()
{
    Scene::SetBackground(ColorF{ 0.16, 0.18, 0.22 });

    const Font titleFont{ 32 };
    const Font font{ 24 };
    constexpr int32 lhs = 20;
    constexpr int32 rhs = 22;
    const int32 result = xlair::sheets::Add(lhs, rhs);

    while (System::Update())
    {
        titleFont(U"SheetsViewer").draw(40, 40, Palette::White);
        font(U"SheetsAnalyzer::Add({}, {}) = {}"_fmt(lhs, rhs, result))
            .draw(40, 96, Palette::Skyblue);
    }
}
