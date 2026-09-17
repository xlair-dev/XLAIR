#include "MetadataLoad.hpp"

#include <utility>

namespace xlair::app {
    namespace {
        void
        ValidateMetadata(const s3d::Array<sheets::Metadata>& metadata, s3d::Array<sheets::Diagnostic>& diagnostics) {
            s3d::HashSet<s3d::String> music_ids;
            s3d::HashSet<s3d::String> difficulty_ids;
            for (const auto& music : metadata) {
                if (music.id.isEmpty()) {
                    diagnostics.push_back(
                        {
                            .severity = sheets::DiagnosticSeverity::Error,
                            .message = U"Music metadata must specify a non-empty ID.",
                            .path = music.source_path,
                        }
                    );
                } else if (!music_ids.insert(music.id).second) {
                    diagnostics.push_back(
                        {
                            .severity = sheets::DiagnosticSeverity::Error,
                            .message = U"Duplicate music ID: " + music.id,
                            .path = music.source_path,
                        }
                    );
                }

                for (const auto& difficulty : music.difficulties) {
                    if (difficulty.id.isEmpty()) {
                        diagnostics.push_back(
                            {
                                .severity = sheets::DiagnosticSeverity::Error,
                                .message = U"Difficulty metadata must specify a non-empty ID.",
                                .path = music.source_path,
                            }
                        );
                    } else if (!difficulty_ids.insert(difficulty.id).second) {
                        diagnostics.push_back(
                            {
                                .severity = sheets::DiagnosticSeverity::Error,
                                .message = U"Duplicate difficulty ID: " + difficulty.id,
                                .path = music.source_path,
                            }
                        );
                    }
                }
            }
        }

        bool HasErrors(const s3d::Array<sheets::Diagnostic>& diagnostics) {
            return diagnostics.any([](const sheets::Diagnostic& diagnostic) {
                return diagnostic.severity == sheets::DiagnosticSeverity::Error;
            });
        }
    }

    MetadataLoad::MetadataLoad(std::unique_ptr<interfaces::IMetadataLoader> loader) : m_loader{ std::move(loader) } {}

    MetadataLoad::~MetadataLoad() {
        if (m_task.isValid()) {
            m_task.wait();
        }
    }

    void MetadataLoad::start() {
        if (m_state == State::Loading) {
            return;
        }

        m_metadata.clear();
        m_diagnostics.clear();
        if (!m_loader) {
            fail(U"A metadata loader is not available.");
            return;
        }

        const auto* loader = m_loader.get();
        m_task = s3d::AsyncTask<LoadResult>{
            [loader]() {
                return loader->load();
            },
        };
        m_state = State::Loading;
    }

    void MetadataLoad::update() {
        if (m_state != State::Loading || !m_task.isReady()) {
            return;
        }

        try {
            auto result = m_task.get();
            m_diagnostics = std::move(result.diagnostics);
            if (!result) {
                m_state = State::Failed;
                return;
            }

            m_metadata = std::move(*result);
            ValidateMetadata(m_metadata, m_diagnostics);
            m_state = HasErrors(m_diagnostics) ? State::Failed : State::Succeeded;
        } catch (...) {
            fail(U"An unexpected error occurred while loading music metadata.");
        }
    }

    MetadataLoad::State MetadataLoad::state() const noexcept {
        return m_state;
    }

    s3d::Array<sheets::Metadata> MetadataLoad::takeMetadata() {
        return std::move(m_metadata);
    }

    const s3d::Array<sheets::Diagnostic>& MetadataLoad::diagnostics() const noexcept {
        return m_diagnostics;
    }

    void MetadataLoad::fail(s3d::String message) {
        m_diagnostics.push_back(
            {
                .severity = sheets::DiagnosticSeverity::Error,
                .message = std::move(message),
            }
        );
        m_state = State::Failed;
    }
}
