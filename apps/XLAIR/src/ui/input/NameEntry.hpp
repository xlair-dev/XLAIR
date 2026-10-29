#pragma once

#include "Common.hpp"

namespace xlair::ui::input {
    class NameEntry {
    public:
        static constexpr std::size_t MaximumLength = 10;

        [[nodiscard]]
        const String& characters() const;

        [[nodiscard]]
        std::size_t selectedIndex() const;

        [[nodiscard]]
        char32 selectedCharacter() const;

        [[nodiscard]]
        String selectedLabel() const;

        [[nodiscard]]
        std::size_t categoryIndex() const;

        [[nodiscard]]
        static constexpr std::size_t categoryCount() {
            return CategoryCount;
        }

        [[nodiscard]]
        StringView categoryLabel() const;

        [[nodiscard]]
        static StringView categoryLabel(std::size_t index);

        bool move(int32 direction);
        bool changeCategory(int32 direction);
        bool appendSelected();
        bool eraseLast();

        [[nodiscard]]
        const String& name() const;

        [[nodiscard]]
        String value() const;

        [[nodiscard]]
        bool valid() const;

        [[nodiscard]]
        std::size_t length() const;

    private:
        static constexpr std::size_t CategoryCount = 5;

        String m_name;
        std::size_t m_selected_index = 0;
    };
}
