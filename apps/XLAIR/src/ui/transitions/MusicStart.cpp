#include "MusicStart.hpp"

#include "ui/components/GameStart.hpp"

namespace xlair::ui::transitions {
    namespace {
        [[nodiscard]]
        double Progress(const double progress) {
            return EaseInOutCubic(Clamp(progress, 0.0, 1.0));
        }
    }

    RectF MusicStart::boundsAt(const double progress) const {
        return source_region.lerp(components::GameStartJacketBounds, Progress(progress));
    }

    RectF MusicStart::imageRegionAt(const double progress) const {
        const auto& target = components::GameStartJacketBounds;
        RectF target_image = target;
        if (jacket && jacket.width() > 0 && jacket.height() > 0) {
            const double scale = Min(target.w / jacket.width(), target.h / jacket.height());
            target_image = RectF{
                Arg::center = target.center(),
                jacket.width() * scale,
                jacket.height() * scale,
            };
        }
        // The menu stretches the full image; only the destination preserves its aspect ratio.
        return source_region.lerp(target_image, Progress(progress));
    }

    void MusicStart::draw(const double progress) const {
        const auto bounds = boundsAt(progress);
        if (!bounds.hasArea()) {
            return;
        }
        bounds.draw(source_background.lerp(ColorF{ 0.05 }, Progress(progress)));
        if (!jacket) {
            return;
        }
        const auto image = imageRegionAt(progress);
        if (image.hasArea()) {
            image(jacket).draw();
        }
    }
}
