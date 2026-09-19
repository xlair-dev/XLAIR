#pragma once

#include "app/Application.hpp"
#include "CatalogSync.hpp"
#include "MetadataLoad.hpp"
#include "app/interfaces/IConfigLoader.hpp"

#include <functional>
#include <memory>

namespace xlair::app::flows {
    class Boot {
    public:
        enum class State {
            LoadingConfig,
            WaitingForSync,
            Syncing,
            SyncFailed,
            LoadingMetadata,
            MetadataFailed,
            Ready,
            Failed,
        };
        using ApiClientFactory = std::function<std::unique_ptr<api::IClient>(const Config::Api&)>;
        using CardReaderFactory = std::function<std::unique_ptr<interfaces::ICardReader>(const Config::CardReader&)>;
        using ControllerDeviceFactory =
            std::function<std::unique_ptr<interfaces::IControllerDevice>(const Config::Controller&)>;

        Boot(
            Application& application,
            std::unique_ptr<interfaces::IConfigLoader> config_loader,
            std::unique_ptr<interfaces::IMetadataLoader> metadata_loader,
            ApiClientFactory api_client_factory,
            CardReaderFactory card_reader_factory,
            ControllerDeviceFactory controller_device_factory,
            CatalogSync::LocalSyncFactory local_sync_factory
        );

        void update(double delta_seconds);
        void skipSync();
        void retrySync();
        void retryMetadata();

        [[nodiscard]]
        inline State state() const noexcept {
            return m_state;
        }

        [[nodiscard]]
        inline double syncWaitRemaining() const noexcept {
            return m_sync_wait_remaining;
        }

        [[nodiscard]]
        inline const Optional<interfaces::ConfigLoadError>& configError() const noexcept {
            return m_config_error;
        }

        [[nodiscard]]
        inline const Optional<api::ApiError>& syncError() const noexcept {
            return m_catalog_sync.error();
        }

        [[nodiscard]]
        inline const Array<sheets::Diagnostic>& metadataDiagnostics() const noexcept {
            return m_metadata_load.diagnostics();
        }

    private:
        void loadConfig();
        void startSync();
        void startMetadataLoad();

        static constexpr double SyncWaitSeconds = 10.0;

        Application& m_application;
        std::unique_ptr<interfaces::IConfigLoader> m_config_loader;
        ApiClientFactory m_api_client_factory;
        CardReaderFactory m_card_reader_factory;
        ControllerDeviceFactory m_controller_device_factory;
        CatalogSync m_catalog_sync;
        MetadataLoad m_metadata_load;
        Optional<interfaces::ConfigLoadError> m_config_error;
        State m_state = State::LoadingConfig;
        double m_sync_wait_remaining = SyncWaitSeconds;
    };
}
