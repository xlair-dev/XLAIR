#pragma once
#include "Common.hpp"
#include "app/interfaces/ICardReader.hpp" // TODO: fix (core -> app)

namespace core::features {
    class CardReaderManager : public IAddon {
    private:
        using CardReader = std::shared_ptr<app::interfaces::ICardReader>;

    public:
        CardReaderManager(CardReader reader);

        ~CardReaderManager();

        static void StartScan();

        static void StopScan();

        static bool IsReady();

        static bool IsOK();

        static String GetID();

        static inline constexpr StringView Name{ U"CardReaderManager" };

    private:
        CardReader m_reader;
    };
}
