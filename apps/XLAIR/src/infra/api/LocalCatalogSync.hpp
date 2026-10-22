#pragma once

#include "app/flows/CatalogSync.hpp"

namespace xlair::infra::api {
    xlair::api::Request<bool> SyncCatalog(
        xlair::api::IClient& client,
        const s3d::Array<xlair::api::Music>& catalog,
        s3d::URLView endpoint,
        s3d::FilePathView sync_directory,
        app::flows::CatalogSync::Mode mode = app::flows::CatalogSync::Mode::Incremental
    );
}
