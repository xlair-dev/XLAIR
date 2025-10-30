#pragma once
#include "Common.hpp"

namespace app::interfaces {
    class ICardReader {
    public:
        virtual ~ICardReader() = default;

        virtual void startScan() = 0;

        virtual void stopScan() = 0;

        virtual bool isReady() const = 0;

        virtual bool isOK() const = 0;

        virtual String getID() const = 0;
    };
}
