#pragma once

#include "app/interfaces/IMetadataLoader.hpp"

#include <memory>

namespace xlair::app {
    class MetadataLoad {
    public:
        enum class State {
            Idle,
            Loading,
            Succeeded,
            Failed,
        };

        explicit MetadataLoad(std::unique_ptr<interfaces::IMetadataLoader> loader);
        ~MetadataLoad();

        MetadataLoad(const MetadataLoad&) = delete;
        MetadataLoad& operator=(const MetadataLoad&) = delete;

        void start();
        void update();

        [[nodiscard]]
        State state() const noexcept;

        [[nodiscard]]
        s3d::Array<sheets::Metadata> takeMetadata();

        [[nodiscard]]
        const s3d::Array<sheets::Diagnostic>& diagnostics() const noexcept;

    private:
        using LoadResult = sheets::Result<s3d::Array<sheets::Metadata>>;

        void fail(s3d::String message);

        std::unique_ptr<interfaces::IMetadataLoader> m_loader;
        s3d::AsyncTask<LoadResult> m_task;
        s3d::Array<sheets::Metadata> m_metadata;
        s3d::Array<sheets::Diagnostic> m_diagnostics;
        State m_state = State::Idle;
    };
}
