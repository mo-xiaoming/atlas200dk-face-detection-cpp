#include "ascend/media.hpp"

namespace hwmedia {
EXCEPTION(Camera_error);

Camera::~Camera() {
    if (!opened_by_me_) {
        return;
    }

    if (auto const e = CloseCamera(channel_id_); e != LIBMEDIA_STATUS_OK) {
        spdlog::error(FMT_STRING("close camera {} failed"), channel_id_);
    }

    spdlog::info(FMT_STRING("camera {} closed"), channel_id_);
}

void Camera::open(Camera::Config conf) {
    conf_ = conf;

    open_camera();
    query_resolutions();
    set_properties(conf);
}

bool Camera::read_frame(void* frame) const {
    auto frame_size = frame_size_yuv420sp(conf_.resolution);
    return ReadFrameFromCamera(channel_id_, frame, &frame_size) == LIBMEDIA_STATUS_OK;
}

void Camera::media_init() {
    if (MediaLibInit() != LIBMEDIA_STATUS_OK) {
        throw Camera_error("media lib initialization failed");
    }
}

void Camera::query_resolutions() {
    std::array<CameraResolution, HIAI_MAX_CAMERARESOLUTION_COUNT> resolutions; // NOLINT
    if (auto const e = GetCameraProperty(channel_id_, CAMERA_PROP_SUPPORTED_RESOLUTION, resolutions.data());
        e != LIBMEDIA_STATUS_OK) {
        spdlog::error(FMT_STRING("data all supported resolutions for camera {} failed"), channel_id_);
        return;
    }
    spdlog::info(FMT_STRING("valid resolutions of camera {}"), channel_id_);
    for (auto const r : resolutions) {
        if (r.width == -1) {
            break;
        }
        spdlog::info(FMT_STRING("    {}x{}"), r.width, r.height);
    }
}

void Camera::open_camera() {
    if (auto const status = QueryCameraStatus(channel_id_); status == CAMERA_STATUS_CLOSED) {
        if (OpenCamera(channel_id_) != LIBMEDIA_STATUS_OK) {
            throw Camera_error(fmt::format(FMT_STRING("open camera {} failed"), channel_id_));
        }
        opened_by_me_ = true;
        spdlog::info(FMT_STRING("camera {} opened"), channel_id_);
    } else if (status != CAMERA_STATUS_OPEN) {
        spdlog::error(FMT_STRING("camera {} in an invalid state {}"), channel_id_, status);
    } else {
        spdlog::info(FMT_STRING("camera {} opened"), channel_id_);
    }
}

void Camera::set_properties(Camera::Config conf) {
    if (auto const e = SetCameraProperty(channel_id_, CAMERA_PROP_FPS, &conf_.fps); e != LIBMEDIA_STATUS_OK) {
        throw Camera_error(fmt::format(FMT_STRING("set camera {} with fps {} failed"), channel_id_, conf_.fps));
    }

    auto const resolution = CameraResolution{.width = conf.resolution.width, .height = conf.resolution.height};
    if (auto const e = SetCameraProperty(channel_id_, CAMERA_PROP_RESOLUTION, &resolution); e != LIBMEDIA_STATUS_OK) {
        throw Camera_error(fmt::format(FMT_STRING("set camera {} with width {} and height {} failed"), channel_id_,
                                       conf.resolution.width, conf.resolution.height));
    }

    auto const mode = CAMERA_CAP_ACTIVE;
    if (auto const e = SetCameraProperty(channel_id_, CAMERA_PROP_CAP_MODE, &mode); e != LIBMEDIA_STATUS_OK) {
        throw Camera_error(fmt::format(FMT_STRING("set camera {} with capture mode to active failed"), channel_id_));
    }
}
} // namespace hwmedia