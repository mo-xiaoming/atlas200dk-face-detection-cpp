#include "ascend/acl.hpp"

#include <spdlog/spdlog.h>

namespace acl::internal::model {

void dataset_add_buf(aclmdlDataset* dataset, void* data, int size) {
    auto* buf = aclCreateDataBuffer(data, static_cast<size_t>(size));
    if (buf == nullptr) {
        throw Acl_error(fmt::format(FMT_STRING("allocate buffer for dataset {} failed"), fmt::ptr(dataset)));
    }
    if (auto const e = aclmdlAddDatasetBuffer(dataset, buf); e != ACL_ERROR_NONE) {
        throw Acl_error(fmt::format(FMT_STRING("add buffer {} to dataset {} failed, code {}"), fmt::ptr(buf),
                                    fmt::ptr(dataset), e));
    }
}

Input_dataset::Dataset::Dataset() {
    if (dataset_ = aclmdlCreateDataset(); dataset_ == nullptr) {
        throw Acl_error("failed to create dateset");
    }
}

Input_dataset::Dataset::~Dataset() {
    auto const n = aclmdlGetDatasetNumBuffers(dataset_);
    assert(n == 1U); // NOLINT
    for (size_t i = 0; i < n; ++i) {
        auto const* buf = aclmdlGetDatasetBuffer(dataset_, i);
        if (auto const e = aclDestroyDataBuffer(buf); e != ACL_ERROR_NONE) {
            spdlog::error(FMT_STRING("failed to destroy data buffer {}, code {}"), fmt::ptr(buf), e);
        }
    }

    if (auto const e = aclmdlDestroyDataset(dataset_); e != ACL_ERROR_NONE) {
        spdlog::error(FMT_STRING("failed to destroy dateset {}, code {}"), fmt::ptr(dataset_), e);
    }
}

Input_dataset::Input_dataset(void* data, int size) { dataset_add_buf(dataset_.get(), data, size); }

Output_dataset::Dataset::Dataset() {
    if (dataset_ = aclmdlCreateDataset(); dataset_ == nullptr) {
        throw Acl_error("failed to create dateset");
    }
}

Output_dataset::Dataset::~Dataset() {
    auto const n = aclmdlGetDatasetNumBuffers(dataset_);
    for (size_t i = 0; i < n; ++i) {
        auto const* buf = aclmdlGetDatasetBuffer(dataset_, i);
        if (auto const e = aclrtFree(aclGetDataBufferAddr(buf)); e != ACL_ERROR_NONE) {
            spdlog::error(FMT_STRING("failed to free {}, code {}"), fmt::ptr(buf), e);
        }
        if (auto const e = aclDestroyDataBuffer(buf); e != ACL_ERROR_NONE) {
            spdlog::error(FMT_STRING("failed to destroy data buffer {}, code {}"), fmt::ptr(buf), e);
        }
    }

    if (auto const e = aclmdlDestroyDataset(dataset_); e != ACL_ERROR_NONE) {
        spdlog::error(FMT_STRING("failed to destroy dateset {}, code {}"), fmt::ptr(dataset_), e);
    }
}

Output_dataset::Output_dataset(aclmdlDesc* desc) {
    auto const n = aclmdlGetNumOutputs(desc);
    spdlog::info(FMT_STRING("total {} output buffers"), n);
    for (size_t i = 0; i < n; ++i) {
        auto const size = aclmdlGetOutputSizeByIndex(desc, i);
        spdlog::info(FMT_STRING("  buffer {} has size {}"), n, size);
        void* data = nullptr;
        if (auto const e = aclrtMalloc(&data, size, ACL_MEM_MALLOC_NORMAL_ONLY); e != ACL_ERROR_NONE) {
            throw Acl_error(fmt::format(FMT_STRING("add buffer of size {} to dateset failed, code {}"), size, e));
        }
        dataset_add_buf(dataset_.get(), data, static_cast<int>(size));
    }
}

Model::Model(std::string_view model_path) {
    if (auto const e = aclmdlLoadFromFile(model_path.data(), &id_); e != ACL_ERROR_NONE) {
        throw Acl_error(fmt::format(FMT_STRING("model file {} fail to load with code {}"), model_path, e));
    }
    spdlog::info(FMT_STRING("[*] model file {} loaded with id {}"), model_path, id_);
}

Model::~Model() {
    spdlog::info(FMT_STRING("[*] unloading model {}"), id_);
    if (auto const e = aclmdlUnload(id_); e != ACL_ERROR_NONE) {
        spdlog::error("model {} failed to unload with code {}", id_, e);
    }
}

Desc::Resource::Resource() {
    if (desc_ = aclmdlCreateDesc(); desc_ == nullptr) {
        throw Acl_error("failed to create model descriptor");
    }
}

Desc::Resource::~Resource() {
    if (auto const e = aclmdlDestroyDesc(desc_); e != ACL_ERROR_NONE) {
        spdlog::error("destroying descriptor failed");
    }
}

Desc::Desc(model::Model_id_type model_id) {
    if (auto const e = aclmdlGetDesc(res_.get(), model_id); e != ACL_ERROR_NONE) {
        throw Acl_error(fmt::format(FMT_STRING("failed to data descriptor for model {}, code {}"), model_id, e));
    }
}
} // namespace acl::internal::model

namespace acl {

bool Model::inference(void* data, int size) const {
    auto input = internal::model::Input_dataset(data, size);
    if (auto const e = aclmdlExecute(model_.id(), input.get(), output_.get()); e != ACL_ERROR_NONE) {
        spdlog::error(FMT_STRING("inference error {}"), e);
        return false;
    }
    return true;
}

void* Model::get_output_buf(int i) const {
    EXCEPTION(Model_result_error);
    if (auto const* buf = aclmdlGetDatasetBuffer(output_.get(), static_cast<unsigned int>(i)); buf != nullptr) {
        if (auto* data = aclGetDataBufferAddr(buf); data != nullptr) {
            return data;
        }
    }
    throw Model_result_error("could not get output data buffer");
}
} // namespace acl