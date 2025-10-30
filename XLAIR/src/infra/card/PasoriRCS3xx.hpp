#pragma once
#include "app/interfaces/ICardReader.hpp"
#include <array>
#include <atomic>
#include <mutex>

namespace infra::card {
    class PasoriRCS3xx : public app::interfaces::ICardReader {
    public:
        using IDm = std::array<uint8, 8>;

        PasoriRCS3xx() = default;

        ~PasoriRCS3xx();

        void startScan() override;

        void stopScan() override;

        bool isReady() const override;

        bool isOK() const override;

        String getID() const override;

    private:
        std::atomic<bool> m_running{ false };
        std::atomic<bool> m_ready{ false };
        std::atomic<bool> m_ok{ true };

        mutable std::mutex m_mutex;
        IDm m_idm{};

        static void Read(PasoriRCS3xx* self);
        AsyncTask<void> m_task;
    };
}
