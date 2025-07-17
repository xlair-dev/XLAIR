#include <Siv3D.hpp> // Siv3D v0.6.16
#include "SheetsAnalyzer.hpp"

void Main() {
    const auto metadata = SheetsAnalyzer::Analyze(U"sheets/example/music.json");
    if (metadata) {
        const auto result = *metadata;
        Print << U"ID: " << result.id;
        Print << U"Title: " << result.title;
        Print << U"Title (Sort): " << result.title_sort;
        Print << U"Artist: " << result.artist;
        Print << U"Genre: " << result.genre;
        Print << U"Music: " << result.music;
        Print << U"Jacket: " << result.jacket;
        Print << U"URL: " << result.url;
        Print << U"Music Offset: " << result.music_offset;
        Print << U"BPM: " << result.bpm;
        Print << U"Difficulties:";
        for (const auto& difficulty : result.difficulties) {
            if (difficulty.src.isEmpty()) {
                continue; // Skip empty difficulties
            }
            Print << U"  Level: " << difficulty.level
                  << U", Source: " << difficulty.src
                  << U", Designer: " << difficulty.desiner;
        }
    }
    while (System::Update()) {
    }
}
