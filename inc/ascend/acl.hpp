#pragma once

#include <string_view>

#include <spdlog/spdlog.h>

#include "acl/acl.h"
#include "acl/ops/acl_dvpp.h"
#include "utility.hpp"

namespace acl {
EXCEPTION(Acl_error);

namespace internal {

struct Acl {
    explicit Acl();
    ~Acl();
    DISABLE_COPY_MOVE(Acl);
};

using Device_id_type = int32_t;

struct Stream {
    Stream();
    ~Stream();
    DISABLE_COPY_MOVE(Stream);

    [[nodiscard]] aclrtStream get() const noexcept { return stream_; }

private:
    aclrtStream stream_ = nullptr;
};

struct Context {
    explicit Context(Device_id_type device_id);
    ~Context();
    DISABLE_COPY_MOVE(Context);

private:
    aclrtContext context_ = nullptr;
    Device_id_type device_id_ = 0;
};

struct Device {
    Device();
    ~Device();
    DISABLE_COPY_MOVE(Device);

    [[nodiscard]] Device_id_type id() const noexcept { return device_id_; }

private:
    Device_id_type const device_id_ = 0;
};
} // namespace internal

struct Initializer {
    explicit Initializer() : context_(device_.id()) {}
    ~Initializer() = default;
    DISABLE_COPY_MOVE(Initializer);

    [[nodiscard]] aclrtStream stream() const noexcept { return stream_.get(); }

private:
    internal::Acl acl_; // NOLINT
    internal::Device device_;
    internal::Context context_; // NOLINT
    internal::Stream stream_;
};

namespace internal::model {

struct Input_dataset {
    struct Dataset {
        Dataset();
        ~Dataset();
        DISABLE_COPY_MOVE(Dataset);

        [[nodiscard]] aclmdlDataset* get() const noexcept { return dataset_; }

    private:
        aclmdlDataset* dataset_ = nullptr;
    };

    Input_dataset(void* data, int size);
    ~Input_dataset() = default;
    DISABLE_COPY_MOVE(Input_dataset);

    [[nodiscard]] aclmdlDataset* get() const noexcept { return dataset_.get(); }

private:
    Dataset dataset_;
};

struct Output_dataset {
    struct Dataset {
        Dataset();
        ~Dataset();
        DISABLE_COPY_MOVE(Dataset);

        [[nodiscard]] aclmdlDataset* get() const noexcept { return dataset_; }

    private:
        aclmdlDataset* dataset_ = nullptr;
    };

    explicit Output_dataset(aclmdlDesc* desc);
    ~Output_dataset() = default;
    DISABLE_COPY_MOVE(Output_dataset);

    [[nodiscard]] aclmdlDataset* get() const noexcept { return dataset_.get(); }

private:
    Dataset dataset_;
};

using Model_id_type = uint32_t;

struct Model {
    explicit Model(std::string_view model_path);
    ~Model();
    DISABLE_COPY_MOVE(Model);

    [[nodiscard]] Model_id_type id() const noexcept { return id_; }

private:
    Model_id_type id_ = 0;
};

struct Desc {
    struct Resource {
        Resource();
        ~Resource();
        DISABLE_COPY_MOVE(Resource);

        [[nodiscard]] aclmdlDesc* get() const noexcept { return desc_; }

    private:
        aclmdlDesc* desc_ = nullptr;
    };

    explicit Desc(Model_id_type model_id);
    ~Desc() = default;
    DISABLE_COPY_MOVE(Desc);

    [[nodiscard]] aclmdlDesc* get() const noexcept { return res_.get(); }

private:
    Resource res_;
};
} // namespace internal::model

struct Model {
    explicit Model(std::string_view model_path) : model_(model_path), desc_(model_.id()), output_(desc_.get()) {}
    ~Model() = default;
    DISABLE_COPY_MOVE(Model);

    [[nodiscard]] bool inference(void* data, int size) const;

    [[nodiscard]] void* get_output_buf(int i) const;

private:
    internal::model::Model model_;
    internal::model::Desc desc_;
    internal::model::Output_dataset output_;
};

namespace internal::dvpp {

struct Channel_desc {
    Channel_desc();
    ~Channel_desc();
    DISABLE_COPY_MOVE(Channel_desc);

    [[nodiscard]] acldvppChannelDesc* get() const noexcept { return desc_; }

private:
    acldvppChannelDesc* desc_ = nullptr;
};

struct Channel {
    explicit Channel(acldvppChannelDesc* desc);
    ~Channel();
    DISABLE_COPY_MOVE(Channel);

    [[nodiscard]] acldvppChannelDesc* desc() const noexcept { return desc_; }

private:
    acldvppChannelDesc* desc_ = nullptr;
};

struct Resize_config {
    Resize_config();
    ~Resize_config();
    DISABLE_COPY_MOVE(Resize_config);

    [[nodiscard]] acldvppResizeConfig* get() const noexcept { return conf_; }

private:
    acldvppResizeConfig* conf_ = nullptr;
};

struct Jpege_config {
    struct Config {
        Config();
        ~Config();
        DISABLE_COPY_MOVE(Config);

        [[nodiscard]] acldvppJpegeConfig* get() const noexcept { return conf_; }

    private:
        acldvppJpegeConfig* conf_ = nullptr;
    };

    Jpege_config();
    ~Jpege_config() = default;
    DISABLE_COPY_MOVE(Jpege_config);

    [[nodiscard]] acldvppJpegeConfig* get() const noexcept { return conf_.get(); }

private:
    Config conf_;
};

struct Pic_desc {
    Pic_desc();
    ~Pic_desc();
    DISABLE_COPY_MOVE(Pic_desc);

    [[nodiscard]] acldvppPicDesc* get() const noexcept { return desc_; }

    void to_yuv420sp(void* data, Resolution resolution);

private:
    acldvppPicDesc* desc_ = nullptr;
};
} // namespace internal::dvpp

struct Dvpp_buffer {
    explicit Dvpp_buffer(int size) : size_(size) { init(size); }
    ~Dvpp_buffer() { destroy(); }
    DISABLE_COPY_MOVE(Dvpp_buffer);

    [[nodiscard]] void* data() const noexcept { return data_; }
    [[nodiscard]] int size() const noexcept { return size_; }

    void resize(int size);

private:
    void init(int size);
    void destroy();

    void* data_ = nullptr;
    int size_ = 0;
};

struct Dvpp {
    explicit Dvpp(Initializer const& initializer) : channel_(desc_.get()), stream_(initializer.stream()){};
    ~Dvpp() = default;
    DISABLE_COPY_MOVE(Dvpp);

    [[nodiscard]] bool resize_yuv420sp(Dvpp_buffer const& input, Resolution input_resolution, Dvpp_buffer const& output,
                                       Resolution output_resolution) const;

    [[nodiscard]] bool yuv420sp_to_jpeg(Dvpp_buffer const& input, Resolution input_resolution,
                                        Dvpp_buffer& output) const;

private:
    internal::dvpp::Channel_desc desc_;
    internal::dvpp::Channel channel_;
    aclrtStream stream_;
};

} // namespace acl
