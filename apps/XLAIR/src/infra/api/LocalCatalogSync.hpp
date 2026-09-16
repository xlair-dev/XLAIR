#pragma once

#include <ApiClient/IClient.hpp>

namespace xlair::infra::api {
    xlair::api::Request<bool> SyncCatalog(
        xlair::api::IClient& client,
        const s3d::Array<xlair::api::Music>& catalog,
        s3d::FilePathView sheets_directory,
        s3d::URLView endpoint
    );
}
