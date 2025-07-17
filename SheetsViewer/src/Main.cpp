#include <Siv3D.hpp> // Siv3D v0.6.16
#include "SheetsAnalyzer.hpp"

void Main() {
    const auto metadata = SheetsAnalyzer::Analyze(U"sheets/example/music.json");
    Print << U"ID: " << metadata.id;
    Print << U"Title: " << metadata.title;
    Print << U"Title (Sort): " << metadata.title_sort;
    Print << U"Artist: " << metadata.artist;
    Print << U"Genre: " << metadata.genre;
    while (System::Update()) {
    }
}
