#include "NameEntry.hpp"

#include <array>

namespace xlair::ui::input {
    namespace {
        struct CharacterCategory {
            StringView label;
            String characters;
        };

        const std::array<CharacterCategory, NameEntry::categoryCount()> Categories{
            CharacterCategory{ U"ABC", U"ABCDEFGHIJKLMNOPQRSTUVWXYZ" },
            CharacterCategory{ U"abc", U"abcdefghijklmnopqrstuvwxyz" },
            CharacterCategory{ U"123", U"0123456789" },
            CharacterCategory{ U"記号", UR"( -_!?@#$%&*+./:;=()[]{}<>~^'"\|,`)" },
            CharacterCategory{
                U"かな",
                U"あいうえおかきくけこさしすせそたちつてとなにぬねのはひふへほまみむめもやゆよらりるれろわをん"
                U"がぎぐげござじずぜぞだぢづでどばびぶべぼぱぴぷぺぽぁぃぅぇぉゃゅょっゎゔー" },
        };

        const String Characters = [] {
            String characters;
            for (const auto& category : Categories) {
                characters += category.characters;
            }
            return characters;
        }();
    }

    const String& NameEntry::characters() const {
        return Characters;
    }

    std::size_t NameEntry::selectedIndex() const {
        return m_selected_index;
    }

    char32 NameEntry::selectedCharacter() const {
        return characters()[selectedIndex()];
    }

    String NameEntry::selectedLabel() const {
        const auto character = selectedCharacter();
        return character == U' ' ? U"SPACE" : String(1, character);
    }

    std::size_t NameEntry::categoryIndex() const {
        std::size_t end = 0;
        for (std::size_t index = 0; index < Categories.size(); ++index) {
            end += Categories[index].characters.size();
            if (m_selected_index < end) {
                return index;
            }
        }
        return Categories.size() - 1;
    }

    StringView NameEntry::categoryLabel() const {
        return categoryLabel(categoryIndex());
    }

    StringView NameEntry::categoryLabel(const std::size_t index) {
        return index < Categories.size() ? Categories[index].label : U"";
    }

    bool NameEntry::move(const int32 direction) {
        const auto next = static_cast<std::size_t>(Clamp(
            static_cast<int64>(m_selected_index) + direction,
            int64{ 0 },
            static_cast<int64>(characters().size() - 1)
        ));
        if (next == m_selected_index) {
            return false;
        }
        m_selected_index = next;
        return true;
    }

    bool NameEntry::changeCategory(const int32 direction) {
        if (direction == 0) {
            return false;
        }
        constexpr auto Count = static_cast<int64>(CategoryCount);
        const auto category =
            static_cast<std::size_t>(((static_cast<int64>(categoryIndex()) + direction) % Count + Count) % Count);
        std::size_t next = 0;
        for (std::size_t index = 0; index < category; ++index) {
            next += Categories[index].characters.size();
        }
        if (next == m_selected_index) {
            return false;
        }
        m_selected_index = next;
        return true;
    }

    bool NameEntry::appendSelected() {
        if (m_name.size() >= MaximumLength) {
            return false;
        }
        m_name.push_back(selectedCharacter());
        return true;
    }

    bool NameEntry::eraseLast() {
        if (m_name.isEmpty()) {
            return false;
        }
        m_name.pop_back();
        return true;
    }

    const String& NameEntry::name() const {
        return m_name;
    }

    String NameEntry::value() const {
        return m_name.trimmed();
    }

    bool NameEntry::valid() const {
        return m_name.any([](const char32 character) {
            return character != U' ';
        });
    }

    std::size_t NameEntry::length() const {
        return m_name.size();
    }
}
