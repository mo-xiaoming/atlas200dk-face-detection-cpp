#pragma once

#include <vector>

#include "ascenddk/presenter/agent/presenter_types.h"

#include "utility.hpp"

namespace ascend::presenter {
class Channel;
}

namespace hwmedia {

struct Presenter {
    explicit Presenter(std::string conf_path);
    ~Presenter() = default;
    DISABLE_COPY_MOVE(Presenter);

    [[nodiscard]] bool show(void* data, int size, Resolution resolution,
              std::vector<ascend::presenter::DetectionResult> results) const;

private:
    ascend::presenter::Channel* channel_ = nullptr;
};
} // namespace hwmedia