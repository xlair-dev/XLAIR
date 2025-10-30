#include "CardReaderManager.hpp"

namespace core::features {
    CardReaderManager::CardReaderManager(CardReader reader)
        :m_reader(std::move(reader)) {
    }

    CardReaderManager::~CardReaderManager() {
        m_reader->stopScan();
    }

    void CardReaderManager::StartScan() {
        if (auto p = Addon::GetAddon<CardReaderManager>(Name)) {
            p->m_reader->startScan();
        }
    }

    void CardReaderManager::StopScan() {
        if (auto p = Addon::GetAddon<CardReaderManager>(Name)) {
            p->m_reader->stopScan();
        }
    }

    bool CardReaderManager::IsReady() {
        if (auto p = Addon::GetAddon<CardReaderManager>(Name)) {
            return p->m_reader->isReady();
        }
        return false;
    }

    bool CardReaderManager::IsOK() {
        if (auto p = Addon::GetAddon<CardReaderManager>(Name)) {
            return p->m_reader->isOK();
        }
        return false;
    }

    String CardReaderManager::GetID() {
        if (auto p = Addon::GetAddon<CardReaderManager>(Name)) {
            return p->m_reader->getID();
        }
        return U"";
    }
}
