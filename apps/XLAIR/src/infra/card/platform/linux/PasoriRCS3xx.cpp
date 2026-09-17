#include "infra/card/PasoriRCS3xx.hpp"

#include "infra/card/detail/UnavailableScan.hpp"

namespace xlair::infra::card {
    app::card::ScanRequest PasoriRCS3xx::scan() {
        return detail::MakeUnavailableScan(U"PaSoRi card reader support is not implemented on Linux yet.");
    }
}
