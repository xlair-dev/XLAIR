#pragma once

#include "app/Application.hpp"
#include "app/interfaces/IConfigWriter.hpp"
#include "CatalogSync.hpp"
#include "MetadataLoad.hpp"

#include <functional>

namespace xlair::app::flows {
    class Maintenance {
    public:
        enum class State {
            Idle,
            CheckingHealth,
            Syncing,
            LoadingMetadata,
            Succeeded,
            Failed,
        };

        using MetadataLoaderFactory = std::function<std::unique_ptr<interfaces::IMetadataLoader>(FilePathView)>;

        Maintenance(
            Application& application,
            MetadataLoaderFactory metadata_loader_factory,
            CatalogSync::LocalSyncFactory local_sync_factory,
            std::unique_ptr<interfaces::IConfigWriter> config_writer
        );
        ~Maintenance();

        void checkHealth();
        void sync(CatalogSync::Mode mode = CatalogSync::Mode::Incremental);
        void saveConfig();
        void update();
        void reset();

        [[nodiscard]]
        State state() const noexcept {
            return m_state;
        }

        [[nodiscard]]
        bool busy() const noexcept;

        [[nodiscard]]
        const String& message() const noexcept {
            return m_message;
        }

        [[nodiscard]]
        const String& configMessage() const noexcept {
            return m_config_message;
        }

        [[nodiscard]]
        bool configSaveFailed() const noexcept {
            return m_config_save_failed;
        }

        [[nodiscard]]
        Optional<api::RequestProgress> progress() const noexcept {
            return m_state == State::Syncing ? m_sync.progress() : none;
        }

    private:
        void fail(String message);

        Application& m_application;
        std::unique_ptr<interfaces::IConfigWriter> m_config_writer;
        String m_config_message;
        bool m_config_save_failed = false;
        MetadataLoaderFactory m_metadata_loader_factory;
        CatalogSync m_sync;
        MetadataLoad m_metadata_load;
        api::Request<bool> m_health_request;
        CatalogSync::Mode m_sync_mode = CatalogSync::Mode::Incremental;
        State m_state = State::Idle;
        String m_message;
    };
}
