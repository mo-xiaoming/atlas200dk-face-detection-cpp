#include <charconv>
#include <csignal>
#include <fstream>
#include <string_view>

#include <spdlog/spdlog.h>

#include "ascend/acl.hpp"
#include "ascend/media.hpp"
#include "ascend/presenter.hpp"
#include "utility.hpp"

using namespace std::literals;

namespace {
volatile std::sig_atomic_t signal_status; // NOLINT

void sigint_handler(int signal) { signal_status = signal; }

EXCEPTION(Invalid_camera_id);

int get_channel_id(std::string_view channel_id_str) {
    int i = 0;
    if (auto const [p, ec] = std::from_chars(channel_id_str.data(), channel_id_str.data() + channel_id_str.size(), i);
        ec != std::errc()) {
        throw Invalid_camera_id(fmt::format(FMT_STRING("invalid camera id {}"), channel_id_str));
    }
    return i;
}

struct Params {
    int camera_channel_id = 0;
};

Params get_params(int argc, char** argv) {
    int channel_id = 1;
    if (argc == 2) {
        channel_id = get_channel_id(argv[1]);
    } else {
        spdlog::info("using default camera 0");
    }

    return Params{.camera_channel_id = channel_id};
}

[[maybe_unused]] void write_frame_to_file(std::string_view path, void* data, int size) {
    auto file = std::ofstream();
    file.open(path.data(), std::ios::out | std::ios::binary);
    file.write(static_cast<char const*>(data), size);
}

struct Bbox {
    float dummy1_ = .0;
    float label_index = 0;
    float score = .0;
    float start_x = .0;
    float start_y = .0;
    float end_x = .0;
    float end_y = .0;
    float dummy2_ = .0;
};
constexpr auto bbox_member_nr = 8;
static_assert(sizeof(Bbox) == sizeof(float) * bbox_member_nr);
static_assert(sizeof(float) == 4);

[[nodiscard]] std::vector<Bbox> get_face_bboxes(acl::Model const& model, Resolution resolution) {
    auto const bbox_nr = *static_cast<uint32_t*>(model.get_output_buf(0));
    auto* data_buf = static_cast<Bbox*>(model.get_output_buf(1));

    constexpr auto face_label_index = 1;
    constexpr auto is_face_threshold = .7F;

    auto bboxes = std::vector<Bbox>();
    bboxes.reserve(bbox_nr);
    for (auto i = 0U; i < bbox_nr; ++i) {
        auto bbox = data_buf[i];
        if (bbox.score < is_face_threshold || std::lround(bbox.label_index) != face_label_index) {
            continue;
        }
        bbox.start_x *= static_cast<float>(resolution.width);
        bbox.start_y *= static_cast<float>(resolution.height);
        bbox.end_x *= static_cast<float>(resolution.width);
        bbox.end_y *= static_cast<float>(resolution.height);
        bboxes.push_back(bbox);
    }
    return bboxes;
}

[[nodiscard]] std::vector<ascend::presenter::DetectionResult> bboxes_to_result(std::vector<Bbox> const& bboxes) {
    auto results = std::vector<ascend::presenter::DetectionResult>();
    results.reserve(bboxes.size());
    std::transform(cbegin(bboxes), cend(bboxes), back_inserter(results), [](Bbox bbox) {
        return ascend::presenter::DetectionResult{
            .lt{.x = static_cast<uint32_t>(bbox.start_x), .y = static_cast<uint32_t>(bbox.start_y)},
            .rb{.x = static_cast<uint32_t>(bbox.end_x), .y = static_cast<uint32_t>(bbox.end_y)},
            .result_text = fmt::format(FMT_STRING("face {:.2f}"), bbox.score)};
    });
    return results;
}

} // namespace

int main(int argc, char** argv) {
    std::signal(SIGINT, sigint_handler);

    try {
        auto const params = get_params(argc, argv);

        auto camera = hwmedia::Camera(params.camera_channel_id);
        constexpr auto capture_resolution = Resolution{.width = 704, .height = 576};
        constexpr auto camera_max_fps = 15;
        camera.open({.fps = camera_max_fps, .resolution = capture_resolution});

        auto const acl_initializer = acl::Initializer();

        auto const dvpp = acl::Dvpp(acl_initializer);
        auto const frame = acl::Dvpp_buffer(frame_size_yuv420sp(capture_resolution));
        constexpr auto model_resolution = Resolution{.width = 304, .height = 300};
        auto const resized_frame = acl::Dvpp_buffer(frame_size_yuv420sp(model_resolution));
        auto jpeg_frame = acl::Dvpp_buffer(frame_size_yuv420sp(capture_resolution));

        auto const model = acl::Model("../model/face_detection.om");

        auto const presenter = hwmedia::Presenter("../data/param.conf");

        while (signal_status == 0) {
            if (!camera.read_frame(frame.data())) {
                continue;
            }

            if (!dvpp.resize_yuv420sp(frame, capture_resolution, resized_frame, model_resolution)) {
                continue;
            }

            if (!model.inference(resized_frame.data(), resized_frame.size())) {
                continue;
            }

            auto const bboxes = get_face_bboxes(model, capture_resolution);
            if (!dvpp.yuv420sp_to_jpeg(frame, capture_resolution, jpeg_frame)) {
                continue;
            }
            if (! presenter.show(jpeg_frame.data(), jpeg_frame.size(), capture_resolution, bboxes_to_result(bboxes))) {
                continue;
            }
        }
    } catch (std::exception const& e) {
        spdlog::error(e.what());
    }
}
