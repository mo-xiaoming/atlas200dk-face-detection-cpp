#pragma once

#include <type_traits>
#include <fmt/format.h>

// NOLINTNEXTLINE
#define EXCEPTION(NAME)                                                                                                \
    struct NAME : std::exception {                                                                                     \
        explicit NAME(std::string msg) : msg_(std::move(msg)) {}                                                       \
        [[nodiscard]] char const* what() const noexcept override { return msg_.c_str(); }                              \
                                                                                                                       \
    private:                                                                                                           \
        std::string msg_;                                                                                              \
    }

// NOLINTNEXTLINE
#define DISABLE_COPY_MOVE(NAME)                                                                                        \
    NAME(NAME const&) = delete;                                                                                        \
    NAME(NAME&&) = delete;                                                                                             \
    NAME& operator=(NAME const&) = delete;                                                                             \
    NAME& operator=(NAME&&) = delete

template <typename T, typename = std::enable_if<std::is_unsigned_v<T>>> [[nodiscard]] T align_up(T n, T a) {
    return ((n - 1) / a + 1) * a;
}

struct Resolution {
    int width = 0;
    int height = 0;
};

template <> struct [[maybe_unused]] fmt::formatter<Resolution> {
    [[maybe_unused]] constexpr auto parse(format_parse_context& ctx) { return ctx.end(); }
    template <typename FormatContext> [[maybe_unused]] auto format(Resolution r, FormatContext& ctx) {
        return format_to(ctx.out(), "(width={}, height={})", r.width, r.height);
    }
};

[[nodiscard]] constexpr int frame_size_yuv420sp(Resolution r) noexcept { return r.width * r.height * 3 / 2; }
