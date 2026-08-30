#include <Siv3D.hpp>

void Main() {
    Scene::SetBackground(ColorF{ 0.16, 0.18, 0.22 });

    const Font titleFont{ 32 };
    const Font font{ 24 };
    constexpr int32 lhs = 20;
    constexpr int32 rhs = 22;

    while (System::Update()) {
        titleFont(U"SheetsViewer").draw(40, 40, Palette::White);
    }
}
