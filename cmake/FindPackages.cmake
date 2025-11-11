# FindPackages.cmake
# Additional package finding logic if needed

# Check for required system libraries
find_package(Threads REQUIRED)

# Check for SIMD support
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-mavx2" COMPILER_SUPPORTS_AVX2)
check_cxx_compiler_flag("/arch:AVX2" COMPILER_SUPPORTS_AVX2_MSVC)

if(COMPILER_SUPPORTS_AVX2 OR COMPILER_SUPPORTS_AVX2_MSVC)
    message(STATUS "AVX2 support detected")
    add_definitions(-DUSE_AVX2)
endif()

