#pragma once

#include <Siv3D.hpp>

namespace xlair::infra::api {
    // Convert one sync-manifest music entry to the existing local metadata v1.
    // The output resides at musics/<id>/music.json.
    s3d::JSON MakeMusicMetadata(const s3d::JSON& music);
}
