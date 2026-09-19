#include "app/flows/Boot.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace xlair::app::flows {
    Boot::Boot(
        Application& application,
        std::unique_ptr<interfaces::IConfigLoader> config_loader,
        std::unique_ptr<interfaces::IMetadataLoader> metadata_loader,
        ApiClientFactory api_client_factory,
        CardReaderFactory card_reader_factory,
        CatalogSync::LocalSyncFactory local_sync_factory
    )
        : m_application{ application }, m_config_loader{ std::move(config_loader) },
          m_api_client_factory{ std::move(api_client_factory) },
          m_card_reader_factory{ std::move(card_reader_factory) }, m_catalog_sync{ std::move(local_sync_factory) },
          m_metadata_load{ std::move(metadata_loader) } {}

    void Boot::update(double delta_seconds) {
        switch (m_state) {
            case State::LoadingConfig:
                loadConfig();
                break;

            case State::WaitingForSync:
                if (std::isfinite(delta_seconds) && delta_seconds > 0.0) {
                    m_sync_wait_remaining = std::max(0.0, m_sync_wait_remaining - delta_seconds);
                }
                if (m_sync_wait_remaining == 0.0) {
                    startSync();
                }
                break;

            case State::Syncing:
                m_catalog_sync.update();
                if (m_catalog_sync.state() == CatalogSync::State::Succeeded) {
                    startMetadataLoad();
                } else if (m_catalog_sync.state() == CatalogSync::State::Failed) {
                    m_state = State::SyncFailed;
                }
                break;

            case State::LoadingMetadata:
                m_metadata_load.update();
                if (m_metadata_load.state() == MetadataLoad::State::Succeeded) {
                    m_application.setMusicCatalog(m_metadata_load.takeMetadata());
                    m_state = State::Ready;
                } else if (m_metadata_load.state() == MetadataLoad::State::Failed) {
                    m_state = State::MetadataFailed;
                }
                break;

            case State::SyncFailed:
            case State::MetadataFailed:
            case State::Ready:
            case State::Failed:
                break;
        }
    }

    void Boot::skipSync() {
        if (m_state == State::WaitingForSync) {
            startMetadataLoad();
        }
    }

    void Boot::retrySync() {
        if (m_state == State::SyncFailed) {
            startSync();
        }
    }

    void Boot::retryMetadata() {
        if (m_state == State::MetadataFailed) {
            startMetadataLoad();
        }
    }

    void Boot::loadConfig() {
        if (!m_config_loader) {
            m_config_error = interfaces::ConfigLoadError{ U"A config loader is not available.", U"" };
            m_state = State::Failed;
            return;
        }

        auto result = m_config_loader->load();
        if (!result) {
            m_config_error = std::move(result.error);
            m_state = State::Failed;
            return;
        }

        auto client = m_api_client_factory ? m_api_client_factory(result.value->api) : nullptr;
        if (!client) {
            m_config_error = interfaces::ConfigLoadError{ U"Failed to initialize the API client.", U"" };
            m_state = State::Failed;
            return;
        }

        auto card_reader = m_card_reader_factory ? m_card_reader_factory(result.value->card_reader) : nullptr;
        if (!card_reader) {
            m_config_error = interfaces::ConfigLoadError{ U"Failed to initialize the card reader.", U"" };
            m_state = State::Failed;
            return;
        }

        m_application.setConfig(std::move(*result.value));
        m_application.setApiClient(std::move(client));
        m_application.setCardReader(std::move(card_reader));
        m_sync_wait_remaining = SyncWaitSeconds;
        m_state = State::WaitingForSync;
    }

    void Boot::startSync() {
        m_catalog_sync.start(*m_application.apiClient(), m_application.config()->api.syncSource());
        m_state = m_catalog_sync.state() == CatalogSync::State::Failed ? State::SyncFailed : State::Syncing;
    }

    void Boot::startMetadataLoad() {
        m_metadata_load.start();
        m_state =
            m_metadata_load.state() == MetadataLoad::State::Failed ? State::MetadataFailed : State::LoadingMetadata;
    }
}
