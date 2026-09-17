#pragma once

#include <SheetsAnalyzer.hpp>

namespace xlair::app::interfaces {
    class IMetadataLoader {
    public:
        virtual ~IMetadataLoader() = default;

        [[nodiscard]]
        virtual sheets::Result<s3d::Array<sheets::Metadata>> load() const = 0;
    };
}
