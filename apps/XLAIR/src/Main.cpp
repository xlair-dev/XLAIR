# include <Siv3D.hpp>
# include "XLAIR/DataDirectory.hpp"

void Main()
{
    Scene::SetBackground(ColorF{ 0.18, 0.20, 0.24 });

    const Font font{ 28 };
    const FilePath dataDirectory = xlair::DataDirectory();

    while (System::Update())
    {
        font(U"XLAIR").draw(40, 40, Palette::White);
        font(U"Edit apps/XLAIR/src/Main.cpp and rebuild").draw(40, 84, Palette::Skyblue);
        font(U"Data: {}"_fmt(dataDirectory)).draw(40, 128, Palette::Lightgray);
    }
}
