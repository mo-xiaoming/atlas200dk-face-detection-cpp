#include "ascend/acl.hpp"

#include <spdlog/spdlog.h>

namespace acl::internal {

Acl::Acl() {
    spdlog::info("[0] acl initializing");
    if (auto const e = aclInit(nullptr); e != ACL_ERROR_NONE) {
        throw Acl_error(fmt::format(FMT_STRING("acl init failed with code {}"), e));
    }
}

Acl::~Acl() {
    spdlog::info("[0] acl finalizing");
    if (auto const e = aclFinalize(); e != ACL_ERROR_NONE) {
        spdlog::error(FMT_STRING("acl finalize failed with code {}"), e);
    }
}

Stream::Stream() {
    spdlog::info("[3] aclrt creating stream");
    if (auto const e = aclrtCreateStream(&stream_); e != ACL_ERROR_NONE) {
        throw Acl_error(fmt::format(FMT_STRING("creating id failed with code {}"), e));
    }
}

Stream::~Stream() {
    spdlog::info("[3] aclrt destroying stream");
    if (auto const e = aclrtDestroyStream(stream_); e != ACL_ERROR_NONE) {
        spdlog::error(FMT_STRING("destroying stream failed with code {}"), e);
    }
}

Context::Context(acl::internal::Device_id_type device_id) : device_id_(device_id) {
    spdlog::info(FMT_STRING("[2] aclrt creating context on device {}"), device_id_);
    if (auto const e = aclrtCreateContext(&context_, device_id); e != ACL_ERROR_NONE) {
        throw Acl_error(fmt::format(FMT_STRING("device {} creates context failed with code {}"), device_id_, e));
    }
}

Context::~Context() {
    spdlog::info(FMT_STRING("[2] aclrt destroying context on device {}"), device_id_);
    if (auto const e = aclrtDestroyContext(context_); e != ACL_ERROR_NONE) {
        spdlog::error(FMT_STRING("context on device {} destroyed failed with code {}"), device_id_, e);
    }
}

Device::Device() {
    spdlog::info(FMT_STRING("[1] acl set device {}"), device_id_);
    if (auto const e = aclrtSetDevice(device_id_); e != ACL_ERROR_NONE) {
        throw Acl_error(fmt::format(FMT_STRING("acl set device {} failed with code {}"), device_id_, e));
    }
}

Device::~Device() {
    spdlog::info(FMT_STRING("[1] acl reset device {}"), device_id_);
    if (auto const e = aclrtResetDevice(device_id_); e != ACL_ERROR_NONE) {
        spdlog::error(FMT_STRING("acl reset device {} failed with code {}"), device_id_, e);
    }
}
} // namespace acl::internal
