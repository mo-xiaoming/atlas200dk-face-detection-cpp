#include "ascend/presenter.hpp"

#include <spdlog/spdlog.h>

#include "ascenddk/presenter/agent/presenter_channel.h"

namespace hwmedia {
EXCEPTION(Presenter_error);

Presenter::Presenter(std::string conf_path) {
    if (auto const e = OpenChannelByConfig(channel_, conf_path.data());
        e != ascend::presenter::PresenterErrorCode::kNone) {
        throw Presenter_error(fmt::format(FMT_STRING("open channel failed, error {}"), e));
    }
    spdlog::info("presenter channel opened");
}

bool Presenter::show(void* data, int size, Resolution resolution,
                     std::vector<ascend::presenter::DetectionResult> results) const {
    auto frame = ascend::presenter::ImageFrame{.format = ascend::presenter::ImageFormat::kJpeg,
                                               .width = static_cast<uint32_t>(resolution.width),
                                               .height = static_cast<uint32_t>(resolution.height),
                                               .size = static_cast<uint32_t>(size),
                                               .data = static_cast<unsigned char*>(data),
                                               .detection_results = std::move(results)};

    if (auto const e = PresentImage(channel_, frame); e != ascend::presenter::PresenterErrorCode::kNone) {
        spdlog::error(FMT_STRING("sent frame to presenter failed, code {}"), e);
        return false;
    }
    return true;
}
} // namespace hwmedia
