#pragma once

#include "Common.hpp"
#include "ViewerSession.hpp"

namespace xlair::sheets_viewer {
    class SideMenu {
    public:
        static constexpr int32 Width = 300;

        void draw(ViewerSession& session, const Rect& area) const;
    };
}
