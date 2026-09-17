#include "CatalogSync.hpp"

#include <utility>

namespace xlair::app {
    CatalogSync::CatalogSync(LocalSyncFactory local_sync_factory)
        : m_local_sync_factory{ std::move(local_sync_factory) } {}

    CatalogSync::~CatalogSync() {
        cancel();
    }

    void CatalogSync::start(api::IClient& client, URLView endpoint) {
        cancel();
        m_client = &client;
        m_endpoint = endpoint;
        m_catalog_request = client.fetchCatalog();
        if (!m_catalog_request) {
            fail({ api::ErrorKind::Configuration, U"The API client did not create a catalog request.", none });
            return;
        }
        m_state = State::Loading;
    }

    void CatalogSync::update() {
        if (m_state != State::Loading) {
            return;
        }

        if (m_local_sync_request) {
            m_local_sync_request->update();
            const auto& result = m_local_sync_request->result();
            if (!result) {
                return;
            }

            if (const auto* error = std::get_if<api::ApiError>(&*result)) {
                fail(*error);
            } else {
                m_pending_catalog.clear();
                m_state = State::Succeeded;
            }
            m_local_sync_request.reset();
            return;
        }

        m_catalog_request->update();
        const auto& result = m_catalog_request->result();
        if (!result) {
            return;
        }

        if (const auto* error = std::get_if<api::ApiError>(&*result)) {
            fail(*error);
        } else {
            m_pending_catalog = std::get<Array<api::Music>>(*result);
            m_local_sync_request =
                m_local_sync_factory ? m_local_sync_factory(*m_client, m_pending_catalog, m_endpoint) : nullptr;
            if (!m_local_sync_request) {
                fail({ api::ErrorKind::Configuration, U"Could not create local catalog sync.", none });
            }
        }
        m_catalog_request.reset();
    }

    void CatalogSync::cancel() {
        if (m_catalog_request) {
            m_catalog_request->cancel();
        }
        if (m_local_sync_request) {
            m_local_sync_request->cancel();
        }
        m_catalog_request.reset();
        m_local_sync_request.reset();
        m_pending_catalog.clear();
        m_client = nullptr;
        m_endpoint.clear();
        m_error.reset();
        m_state = State::Idle;
    }
}
