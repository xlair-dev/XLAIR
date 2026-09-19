#include "Assets.hpp"

namespace xlair::ui::assets {
    namespace {
        void Require(const bool succeeded, const StringView message) {
            if (!succeeded) {
                throw Error{ message };
            }
        }

        void InitializeFonts() {
            Require(
                FontAsset::Register(font::Label, FontMethod::MSDF, 38, Resource(U"ui/fonts/Jost/Jost-Medium.ttf")),
                U"Failed to register the UI label font."
            );
            Require(FontAsset::Load(font::Label), U"Failed to load the UI label font.");

            Require(
                FontAsset::Register(font::Text, FontMethod::MSDF, 75, Resource(U"ui/fonts/Jost/Jost-Regular.ttf")),
                U"Failed to register the UI text font."
            );
            Require(FontAsset::Load(font::Text), U"Failed to load the UI text font.");

            Require(
                FontAsset::Register(
                    font::Display,
                    FontMethod::MSDF,
                    68,
                    Resource(U"ui/fonts/BrunoAce/BrunoAce-Regular.ttf")
                ),
                U"Failed to register the display font."
            );
            Require(FontAsset::Load(font::Display), U"Failed to load the display font.");

            Require(
                FontAsset::Register(
                    font::ComboNumber,
                    FontMethod::Bitmap,
                    270,
                    Resource(U"ui/fonts/Jost/Jost-Regular.ttf")
                ),
                U"Failed to register the combo number font."
            );
            Require(FontAsset::Load(font::ComboNumber, U"0123456789"), U"Failed to load the combo number font.");

            Require(
                FontAsset::Register(font::CjkFallback, FontMethod::MSDF, 75, Typeface::CJK_Regular_JP),
                U"Failed to register the CJK fallback font."
            );
            Require(FontAsset::Load(font::CjkFallback), U"Failed to load the CJK fallback font.");

            const Font cjk_fallback = FontAsset{ font::CjkFallback };
            Require(FontAsset{ font::Label }.addFallback(cjk_fallback), U"Failed to configure the UI label font.");
            Require(FontAsset{ font::Text }.addFallback(cjk_fallback), U"Failed to configure the UI text font.");
        }

        void InitializeTextures() {
            Require(
                TextureAsset::Register(texture::Logo, Resource(U"ui/textures/logo.png"), TextureDesc::Mipped),
                U"Failed to register the XLAIR logo."
            );
            Require(TextureAsset::Load(texture::Logo), U"Failed to load the XLAIR logo.");
        }
    }

    void Initialize() {
        InitializeFonts();
        InitializeTextures();
    }
}
