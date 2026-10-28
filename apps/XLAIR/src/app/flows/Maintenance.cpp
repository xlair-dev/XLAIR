#include "Maintenance.hpp"

namespace xlair::app::flows {
    Maintenance::Maintenance(
        Application& application,
        MetadataLoaderFactory metadata_loader_factory,
        CatalogSync::LocalSyncFactory local_sync_factory,
        std::unique_ptr<interfaces::IConfigWriter> config_writer
    )
        : m_application{ application }, m_config_writer{ std::move(config_writer) },
          m_metadata_loader_factory{ std::move(metadata_loader_factory) }, m_sync{ std::move(local_sync_factory) },
          m_metadata_load{ nullptr } {}

    Maintenance::~Maintenance() {
        reset();
    }

    bool Maintenance::busy() const noexcept {
        return m_state == State::CheckingHealth || m_state == State::Syncing || m_state == State::LoadingMetadata;
    }

    void Maintenance::saveConfig() {
        if (busy()) {
            return;
        }
        m_config_save_failed = false;
        if (!m_application.hasUnsavedConfig()) {
            m_config_message = U"保存する変更はありません。";
            return;
        }
        if (!m_config_writer) {
            m_config_save_failed = true;
            m_config_message = U"設定の保存先が利用できません。";
            return;
        }
        const auto& current = *m_application.config();
        if (const auto error = m_config_writer->save(*m_application.m_saved_config, current)) {
            m_config_save_failed = true;
            m_config_message = U"設定を保存できませんでした。\n{}\n{}"_fmt(error->message, error->path);
            Logger << U"[Maintenance] " + m_config_message;
            return;
        }
        m_application.m_saved_config = current;
        m_config_message = U"設定を保存しました。";
    }

    void Maintenance::checkHealth() {
        if (busy()) {
            return;
        }
        reset();
        if (auto* client = m_application.apiClient()) {
            m_health_request = client->checkHealth();
        }
        if (!m_health_request) {
            fail(U"API client is not available.");
            return;
        }
        m_state = State::CheckingHealth;
        m_message = U"Checking API health...";
    }

    void Maintenance::sync(const CatalogSync::Mode mode) {
        if (busy()) {
            return;
        }
        reset();
        const auto& config = m_application.config();
        auto* client = m_application.apiClient();
        if (!config || !client) {
            fail(U"API client is not available.");
            return;
        }
        auto loader = m_metadata_loader_factory ? m_metadata_loader_factory(config->sync.directory) : nullptr;
        if (!loader) {
            fail(U"Metadata loader is not available.");
            return;
        }
        m_metadata_load.setLoader(std::move(loader));
        m_sync_mode = mode;
        m_sync.start(*client, config->api.syncSource(), config->sync.directory, mode);
        m_state = State::Syncing;
        m_message = mode == CatalogSync::Mode::VerifyAll
                        ? U"Verifying all assets and repairing missing or damaged files..."
                        : U"Syncing catalog and assets...";
    }

    void Maintenance::update() {
        if (m_state == State::CheckingHealth) {
            m_health_request->update();
            const auto& result = m_health_request->result();
            if (!result) {
                return;
            }
            if (const auto* error = std::get_if<api::ApiError>(&*result)) {
                fail(error->message);
            } else if (!std::get<bool>(*result)) {
                fail(U"API health check failed.");
            } else {
                m_state = State::Succeeded;
                m_message = U"API health: OK";
            }
            m_health_request.reset();
        } else if (m_state == State::Syncing) {
            m_sync.update();
            if (m_sync.state() == CatalogSync::State::Failed) {
                fail(m_sync.error()->message);
            } else if (m_sync.state() == CatalogSync::State::Succeeded) {
                m_metadata_load.start();
                m_state = State::LoadingMetadata;
                m_message = U"Loading metadata...";
            }
        } else if (m_state == State::LoadingMetadata) {
            m_metadata_load.update();
            if (m_metadata_load.state() == MetadataLoad::State::Failed) {
                const auto& diagnostics = m_metadata_load.diagnostics();
                for (const auto& diagnostic : diagnostics) {
                    Logger << U"[Maintenance] {}: {}"_fmt(diagnostic.path, diagnostic.message);
                }
                fail(
                    diagnostics.isEmpty() ? U"Failed to load metadata."
                                          : diagnostics.front().path + U": " + diagnostics.front().message
                );
            } else if (m_metadata_load.state() == MetadataLoad::State::Succeeded) {
                m_application.setMusicCatalog(m_metadata_load.takeMetadata());
                m_state = State::Succeeded;
                m_message = m_sync_mode == CatalogSync::Mode::VerifyAll ? U"Sync and full verification completed."
                                                                        : U"Sync completed.";
            }
        }
    }

    void Maintenance::reset() {
        if (m_health_request) {
            m_health_request->cancel();
            m_health_request.reset();
        }
        m_sync.cancel();
        m_state = State::Idle;
        m_message.clear();
        m_config_message.clear();
        m_config_save_failed = false;
    }

    void Maintenance::fail(String message) {
        m_state = State::Failed;
        m_message = std::move(message);
        Logger << U"[Maintenance] " + m_message;
    }
}
