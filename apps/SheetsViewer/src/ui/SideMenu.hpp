#pragma once

#include "ViewerSession.hpp"

namespace xlair::sheets_viewer {
    class SideMenu {
    public:
        static constexpr s3d::int32 Width = 300;

        void draw(ViewerSession& session, const s3d::Rect& area) const;
    };
}
