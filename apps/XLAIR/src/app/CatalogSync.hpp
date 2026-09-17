#pragma once

#include "Common.hpp"

#include <ApiClient/IClient.hpp>

#include <functional>

namespace xlair::app {
    class CatalogSync {
    public:
        enum class State { Idle, Loading, Succeeded, Failed };
        using LocalSyncFactory = std::function<api::Request<bool>(api::IClient&, const Array<api::Music>&, URLView)>;

        explicit CatalogSync(LocalSyncFactory local_sync_factory);
        ~CatalogSync();

        void start(api::IClient& client, URLView endpoint);
        void update();
        void cancel();

        [[nodiscard]]
        inline State state() const noexcept {
            return m_state;
        }

        [[nodiscard]]
        inline const Optional<api::ApiError>& error() const noexcept {
            return m_error;
        }

    private:
        inline void fail(api::ApiError error) {
            m_error = std::move(error);
            m_state = State::Failed;
        }

        LocalSyncFactory m_local_sync_factory;
        api::IClient* m_client = nullptr;
        URL m_endpoint;
        api::Request<Array<api::Music>> m_catalog_request;
        api::Request<bool> m_local_sync_request;
        Array<api::Music> m_pending_catalog;
        Optional<api::ApiError> m_error;
        State m_state = State::Idle;
    };
}
