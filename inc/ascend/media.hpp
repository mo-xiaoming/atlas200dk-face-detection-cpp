#pragma once

#include <spdlog/spdlog.h>

extern "C" {
#include "driver/peripheral_api.h"
}

#include "utility.hpp"

namespace hwmedia {

struct Camera {
    struct Config {
        static constexpr int max_fps = 20;
        static constexpr int default_width = 1280;
        static constexpr int default_height = 720;

        int fps = max_fps;
        Resolution resolution = {.width = default_width, .height = default_height};
    };

    explicit Camera(int channel_id) : channel_id_(channel_id) { media_init(); }
    ~Camera();
    DISABLE_COPY_MOVE(Camera);

    void open(Config conf);
    bool read_frame(void* frame) const;

private:
    void media_init();
    void query_resolutions();
    void open_camera();
    void set_properties(Config conf);

    int channel_id_ = 0;
    bool opened_by_me_ = false;
    Config conf_;
};
} // namespace hwmedia