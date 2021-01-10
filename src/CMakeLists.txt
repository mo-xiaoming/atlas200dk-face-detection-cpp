set(CMAKE_C_COMPILER /usr/bin/aarch64-linux-gnu-gcc-10)
set(CMAKE_CXX_COMPILER /usr/bin/aarch64-linux-gnu-g++-10)

include(${CMAKE_BINARY_DIR}/conan_paths.cmake)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "../../out")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wfatal-errors")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=native -mtune=native")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-optimize-sibling-calls -fno-omit-frame-pointer")

if (CMAKE_BUILD_TYPE STREQUAL Debug)
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address,undefined")
endif ()

set(cxx_strict_flags "-Wcast-align -Wcast-qual -Wconversion -Wdouble-promotion -Wduplicated-branches -Wduplicated-cond -Wfloat-equal -Wformat=2 -Wlogical-op -Wmisleading-indentation -Wmissing-include-dirs -Wnon-virtual-dtor -Wnull-dereference -Wold-style-cast -Woverloaded-virtual -Wpointer-arith -Wredundant-decls -Wshadow -Wsign-conversion -Wswitch-enum -Wtrampolines -Wundef -Wunused -Wuseless-cast -Wzero-as-null-pointer-constant")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${cxx_strict_flags} -fPIC")

if (CMAKE_BUILD_TYPE STREQUAL Release)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
endif ()

add_executable(main
        main.cpp
        ascend/acl.cpp
        ascend/acl_model.cpp
        ascend/acl_dvpp.cpp
        ascend/media.cpp
        ascend/presenter.cpp
)

target_compile_definitions(main PRIVATE -DENABLE_DVPP_INTERFACE)

target_include_directories(main SYSTEM PRIVATE
        $ENV{HOME}/ascend_ddk/arm/include/ #presenter opencv ffmpeg and encoding libs
        $ENV{HOME}/atlas200dk/ # driver
        $ENV{HOME}/acl/acllib/include/ #acl
)
target_include_directories(main PRIVATE ../inc/)

target_link_directories(main PRIVATE
        $ENV{HOME}/atlas200dk/driver/
        $ENV{HOME}/ascend_ddk/arm/lib/
        $ENV{HOME}/Ascend/acllib/lib64/
)

find_package(spdlog REQUIRED)
target_link_libraries(main ascendcl acl_dvpp media_mini presenteragent spdlog::spdlog)

install(TARGETS main DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})