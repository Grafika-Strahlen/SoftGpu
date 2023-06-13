#pragma once

#include <DynArray.hpp>

namespace tau::vd {

[[nodiscard]] DynArray<const char*> GetRequestedInstanceLayers(u32* const layerCount) noexcept;

}
