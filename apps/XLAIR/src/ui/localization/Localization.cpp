#include "Localization.hpp"

#include <array>

namespace xlair::ui::localization {
    namespace {
        struct TextEntry {
            TextId id;
            StringView key;
        };

        constexpr std::array TextEntries{
#define X(name, key) TextEntry{ TextId::name, key },
#include "TextList.inc"
#undef X
        };

        std::array<String, static_cast<std::size_t>(TextId::Count)> strings;
        bool initialized = false;

        [[nodiscard]]
        constexpr std::size_t ToIndex(const TextId id) noexcept {
            return static_cast<std::size_t>(id);
        }
    }

    void Initialize() {
        initialized = false;
        strings = {};

        const FilePath path = Resource(U"ui/locales/ja.toml");
        const TOMLReader toml{ path };
        if (!toml) {
            throw Error{ U"Failed to load the localization catalog: {}"_fmt(path) };
        }

        for (const auto& entry : TextEntries) {
            const auto value = toml[String{ entry.key }].getOpt<String>();
            if (!value) {
                throw Error{ U"Localization text '{}' is missing or invalid in '{}'."_fmt(entry.key, path) };
            }
            strings[ToIndex(entry.id)] = *value;
        }

        initialized = true;
    }

    const String& GetText(const TextId id) {
        const std::size_t index = ToIndex(id);
        if (!initialized || index >= strings.size()) {
            throw Error{ U"Localization has not been initialized or the text ID is invalid." };
        }
        return strings[index];
    }
}
