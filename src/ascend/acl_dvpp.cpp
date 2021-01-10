#include "ascend/acl.hpp"

#include <spdlog/spdlog.h>

namespace acl::internal::dvpp {

Channel_desc::Channel_desc() {
    if (desc_ = acldvppCreateChannelDesc(); desc_ == nullptr) {
        throw Acl_error("create dvpp descriptor failed");
    }
}

Channel_desc::~Channel_desc() {
    if (auto const e = acldvppDestroyChannelDesc(desc_); e != ACL_ERROR_NONE) {
        spdlog::error(FMT_STRING("destroy dvpp descriptor failed, code {}"), e);
    }
}

Channel::Channel(acldvppChannelDesc* desc) : desc_(desc) {
    if (auto const e = acldvppCreateChannel(desc_); e != ACL_ERROR_NONE) {
        throw Acl_error(fmt::format(FMT_STRING("create dvpp channel failed, code {}"), e));
    }
}

Channel::~Channel() {
    if (auto const e = acldvppDestroyChannel(desc_); e != ACL_ERROR_NONE) {
        spdlog::error(FMT_STRING("destroy channel failed, code {}"), e);
    }
}

Resize_config::Resize_config() {
    if (conf_ = acldvppCreateResizeConfig(); conf_ == nullptr) {
        throw Acl_error("dvpp create resize config failed");
    }
}

Resize_config::~Resize_config() {
    if (auto const e = acldvppDestroyResizeConfig(conf_); e != ACL_ERROR_NONE) {
        spdlog::error("failed to destroy dvpp resize config");
    }
}

Jpege_config::Config::Config() {
    if (conf_ = acldvppCreateJpegeConfig(); conf_ == nullptr) {
        throw Acl_error("dvpp create jpege config failed");
    }
}

Jpege_config::Config::~Config() {
    if (auto const e = acldvppDestroyJpegeConfig(conf_); e != ACL_ERROR_NONE) {
        spdlog::error("failed to destroy dvpp jpege config");
    }
}

Jpege_config::Jpege_config() {
    constexpr auto highest_quality = 100;
    if (auto const e = acldvppSetJpegeConfigLevel(conf_.get(), static_cast<uint32_t>(highest_quality));
        e != ACL_ERROR_NONE) {
        spdlog::error(FMT_STRING("failed to set jpege quality to {}"), highest_quality);
    }
}

Pic_desc::Pic_desc() {
    if (desc_ = acldvppCreatePicDesc(); desc_ == nullptr) {
        throw Acl_error("dvpp create pic desc failed");
    }
}

Pic_desc::~Pic_desc() {
    if (auto const e = acldvppDestroyPicDesc(desc_); e != ACL_ERROR_NONE) {
        spdlog::error("failed to destroy dvpp pic desc");
    }
}

void Pic_desc::to_yuv420sp(void* data, Resolution resolution) {
    constexpr auto width_align = 16U;
    constexpr auto height_align = 2U;

    auto const aligned_resolution =
        Resolution{.width = static_cast<int>(align_up(static_cast<unsigned int>(resolution.width), width_align)),
                   .height = static_cast<int>(align_up(static_cast<unsigned int>(resolution.height), height_align))};
    auto const aligned_frame_size = static_cast<unsigned int>(frame_size_yuv420sp(aligned_resolution));

    if (auto const e = acldvppSetPicDescData(desc_, data); e != ACL_ERROR_NONE) {
        throw Acl_error(fmt::format(FMT_STRING("set data {} to pic desc failed"), data));
    }
    if (auto const e = acldvppSetPicDescFormat(desc_, PIXEL_FORMAT_YUV_SEMIPLANAR_420); e != ACL_ERROR_NONE) {
        throw Acl_error("set format PIXEL_FORMAT_YUV_SEMIPLANAR_420 to pic desc failed");
    }
    if (auto const e = acldvppSetPicDescWidth(desc_, static_cast<unsigned int>(resolution.width));
        e != ACL_ERROR_NONE) {
        throw Acl_error(fmt::format(FMT_STRING("set width {} to pic desc failed"), resolution.width));
    }
    if (auto const e = acldvppSetPicDescHeight(desc_, static_cast<unsigned int>(resolution.height));
        e != ACL_ERROR_NONE) {
        throw Acl_error(fmt::format(FMT_STRING("set height {} to pic desc failed"), resolution.height));
    }
    if (auto const e = acldvppSetPicDescWidthStride(desc_, static_cast<unsigned int>(aligned_resolution.width));
        e != ACL_ERROR_NONE) {
        throw Acl_error(fmt::format(FMT_STRING("set width stride {} to pic desc failed"),
                                    static_cast<unsigned int>(aligned_resolution.width)));
    }
    if (auto const e = acldvppSetPicDescHeightStride(desc_, static_cast<unsigned int>(aligned_resolution.height));
        e != ACL_ERROR_NONE) {
        throw Acl_error(fmt::format(FMT_STRING("set height stride {} to pic desc failed"),
                                    static_cast<unsigned int>(aligned_resolution.height)));
    }
    if (auto const e = acldvppSetPicDescSize(desc_, aligned_frame_size); e != ACL_ERROR_NONE) {
        throw Acl_error(fmt::format(FMT_STRING("set size {} to pic desc failed"), aligned_frame_size));
    }
}
} // namespace acl::internal::dvpp

namespace acl {

bool Dvpp::resize_yuv420sp(const acl::Dvpp_buffer& input, Resolution input_resolution, const acl::Dvpp_buffer& output,
                           Resolution output_resolution) const {
    auto input_pic_desc = internal::dvpp::Pic_desc();
    auto output_pic_desc = internal::dvpp::Pic_desc();

    input_pic_desc.to_yuv420sp(input.data(), input_resolution);
    output_pic_desc.to_yuv420sp(output.data(), output_resolution);

    auto const config = internal::dvpp::Resize_config();

    if (auto const e =
            acldvppVpcResizeAsync(channel_.desc(), input_pic_desc.get(), output_pic_desc.get(), config.get(), stream_);
        e != ACL_ERROR_NONE) {
        spdlog::error(FMT_STRING("failed to start resizing image from {} to {} in yuv420sp"), input_resolution,
                      output_resolution);
        return false;
    }
    if (auto const e = aclrtSynchronizeStream(stream_); e != ACL_ERROR_NONE) {
        spdlog::error(FMT_STRING("failed to wait for resizing image form {} to {} yuv420sp finish"), input_resolution,
                      output_resolution);
        return false;
    }
    return true;
}

bool Dvpp::yuv420sp_to_jpeg(const acl::Dvpp_buffer& input, Resolution input_resolution,
                            acl::Dvpp_buffer& output) const {
    auto input_pic_desc = internal::dvpp::Pic_desc();
    input_pic_desc.to_yuv420sp(input.data(), input_resolution);

    auto const config = internal::dvpp::Jpege_config();

    uint32_t out_size = 0;
    if (auto const e = acldvppJpegPredictEncSize(input_pic_desc.get(), config.get(), &out_size); e != ACL_ERROR_NONE) {
        spdlog::error(FMT_STRING("failed to predict image size from {} yuv420sp"), input_resolution);
        return false;
    }
    output.resize(static_cast<int>(out_size));

    if (auto const e = acldvppJpegEncodeAsync(channel_.desc(), input_pic_desc.get(), output.data(), &out_size,
                                              config.get(), stream_);
        e != ACL_ERROR_NONE) {
        spdlog::error("failed to convert yuv420sp to jpeg");
        return false;
    }
    if (auto const e = aclrtSynchronizeStream(stream_); e != ACL_ERROR_NONE) {
        spdlog::error("failed to wait for converting yuv420sp to jpeg finish");
        return false;
    }
    return true;
}

void Dvpp_buffer::resize(int size) {
    if (size <= size_) {
        return;
    }

    destroy();
    size_ = size;
    init(size);
}

void Dvpp_buffer::init(int size) {
    if (auto const e = acldvppMalloc(&data_, static_cast<size_t>(size)); e != ACL_ERROR_NONE) {
        throw Acl_error(fmt::format(FMT_STRING("dvpp allocate {} bytes failed, code {}"), size, e));
    }
}

void Dvpp_buffer::destroy() {
    if (auto const e = acldvppFree(data_); e != ACL_ERROR_NONE) {
        spdlog::error(FMT_STRING("dvpp free {} failed"), data_);
    }
}
} // namespace acl