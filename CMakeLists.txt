cmake_minimum_required(VERSION 3.13)

project(jpeg-decoder)

set(CMAKE_MODULE_PATH              "${CMAKE_SOURCE_DIR}/cmake")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_EXPORT_COMPILE_COMMANDS  ON)

include(cmake/BuildFlags.cmake)
include(cmake/TestSolution.cmake)
include(cmake/SeminarUtils.cmake)

include(FetchContent)
include(cmake/FindGlog.cmake)

find_package(Catch REQUIRED)
find_package(Benchmark REQUIRED)
find_package(GTest REQUIRED)

add_subdirectory(allocations_checker)

find_package(PNG)
find_package(JPEG)
find_package(FFTW)

# If you have problems with libraries
# (paths below are displayed empty in CMake log or build does not work),
# then you are possibly on MacOS. First, uncomment these lines and retry.
# If this did not help, try to find path to your libs by running
# `brew --prefix fftw` and replacing /usr/local/opt/fftw below with your location.
# (And respectively with libpng and jpeg.)

# set(PNG_INCLUDE_DIRS  /usr/local/opt/libpng/include)
# set(PNG_LIBRARY       /usr/local/opt/libpng/lib/libpng.dylib)
# set(JPEG_INCLUDE_DIRS /usr/local/opt/jpeg/include)
# set(JPEG_LIBRARIES    /usr/local/opt/jpeg/libjpeg.dylib)
# set(FFTW_INCLUDES     /usr/local/opt/fftw/include)
# set(FFTW_LIBRARIES    /usr/local/opt/fftw/lib/libfftw3.dylib)

set(JPEG_DECODER_DEPS_PRESENT TRUE)

if (${PNG_FOUND})
    message(STATUS "Path to PNG includes: ${PNG_INCLUDE_DIRS}")
    message(STATUS "Path to PNG library: ${PNG_LIBRARY}")
else (${PNG_FOUND})
    message(WARNING "PNG library was not found. Jpeg-Decoder task will not be configured")
    set(JPEG_DECODER_DEPS_PRESENT FALSE)
endif()

if (${JPEG_FOUND})
    message(STATUS "Path to JPEG includes: ${JPEG_INCLUDE_DIRS}")
    message(STATUS "Path to JPEG library: ${JPEG_LIBRARIES}")
else (${JPEG_FOUND})
    message(WARNING "JPEG library was not found. Jpeg-Decoder task will not be configured")
    set(JPEG_DECODER_DEPS_PRESENT FALSE)
endif()

if (${FFTW_FOUND})
    message(STATUS "Path to FFTW includes: ${FFTW_INCLUDES}")
    message(STATUS "Path to FFTW library: ${FFTW_LIBRARIES}")
else (${FFTW_FOUND})
    message(WARNING "FFTW library was not found. Jpeg-Decoder task will not be configured")
    set(JPEG_DECODER_DEPS_PRESENT FALSE)
endif()

if (NOT ${JPEG_DECODER_DEPS_PRESENT})
    return()
endif()

set(DECODER_UTIL_FILES
    utils/logger_init.cpp
    utils/libjpg_reader.cpp
    utils/png_encoder.cpp 
    utils/test_commons.cpp
)

add_catch(test_decoder_baseline
    baseline/tests/test_baseline.cpp
    ${DECODER_UTIL_FILES}
)

target_compile_definitions(test_decoder_baseline PUBLIC HSE_TASK_DIR="${CMAKE_CURRENT_SOURCE_DIR}/")

if (GRADER)
    target_compile_definitions(test_decoder_baseline PUBLIC HSE_ARTIFACTS_DIR="/tmp/artifacts")
endif ()

set(DECODER_UTILS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/utils)

function(link_decoder_deps TARGET)
    target_include_directories(${TARGET} PUBLIC
            ${PNG_INCLUDE_DIRS}
            ${FFTW_INCLUDES}
            ${JPEG_INCLUDE_DIRS}
            ${DECODER_UTILS_DIR})

    target_link_libraries(${TARGET} PUBLIC
            ${FFTW_LIBRARIES}
            ${PNG_LIBRARY}
            ${JPEG_LIBRARIES}
            glog::glog)

#    get_target_property(GLOG_INCLUDES glog::glog INCLUDE_DIRECTORIES)
#    target_include_directories(${TARGET} SYSTEM PUBLIC ${GLOG_INCLUDES})

endfunction()

add_subdirectory(baseline)


if (NOT CMAKE_CXX_COMPILER_ID MATCHES "^Clang$")
    message(WARNING "Clang is required for fuzzing tests (Apple Clang does not fit too). This is just warning, you can use your current compiler for all tasks except fuzzing. Go to tasks/jpeg-decoder/README.md for guide about clang installation.")
else()
    add_executable(fuzz_decoder_baseline baseline/tests/fuzz_jpeg.cpp)
    set_property(TARGET fuzz_decoder_baseline APPEND PROPERTY COMPILE_OPTIONS "-fsanitize=fuzzer-no-link")
    target_link_libraries(fuzz_decoder_baseline decoder_baseline ${FFTW_LIBRARIES} "-fsanitize=fuzzer")
    if (MAX_ALLOWED_IMAGE_SIZE_BYTES)
        message(STATUS "Using MAX_ALLOWED_IMAGE_SIZE_BYTES=${MAX_ALLOWED_IMAGE_SIZE_BYTES} for fuzzing baseline & faster targets")
        target_compile_definitions(fuzz_decoder_baseline PUBLIC MAX_ALLOWED_IMAGE_SIZE_BYTES=${MAX_ALLOWED_IMAGE_SIZE_BYTES})
    endif()
endif ()
