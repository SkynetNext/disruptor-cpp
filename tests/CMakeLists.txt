# Use Google Test from submodule
set(GOOGLETEST_ROOT ${CMAKE_SOURCE_DIR}/third_party/googletest)

if(EXISTS ${GOOGLETEST_ROOT}/CMakeLists.txt)
    message(STATUS "Using Google Test submodule at ${GOOGLETEST_ROOT}")
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    # Disable Google Test's own tests and examples
    set(gtest_build_tests OFF CACHE BOOL "" FORCE)
    set(gtest_build_samples OFF CACHE BOOL "" FORCE)
    set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)

    add_subdirectory(${GOOGLETEST_ROOT} ${CMAKE_BINARY_DIR}/googletest-build EXCLUDE_FROM_ALL)
else()
    message(FATAL_ERROR "Google Test submodule not found. Please run: git submodule update --init --recursive")
endif()

# Collect all ported Java tests (C++ gtest) under tests/disruptor/**.
file(GLOB_RECURSE DISRUPTOR_PORTED_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/disruptor/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/disruptor/*/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/disruptor/*/*/*.cpp"
)

if(DISRUPTOR_PORTED_TEST_SOURCES)
    add_executable(disruptor_cpp_tests
        ${DISRUPTOR_PORTED_TEST_SOURCES}
    )

    target_link_libraries(disruptor_cpp_tests
        PRIVATE
            disruptor-cpp
            gtest
            gtest_main
    )

    target_include_directories(disruptor_cpp_tests
        PRIVATE
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_SOURCE_DIR}
    )

    # Compiler warnings: treat warnings as errors for project code
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(disruptor_cpp_tests PRIVATE 
            -Werror
            -Wno-unused-parameter
        )
    elseif(MSVC)
        target_compile_options(disruptor_cpp_tests PRIVATE 
            /WX  # Treat warnings as errors
            /wd4100  # Suppress unused parameter warnings
        )
    endif()

    include(GoogleTest)
    
    # Check if MSan is enabled (MemorySanitizer interferes with gtest_discover_tests)
    # MSan adds -fsanitize=memory to compiler/linker flags
    # Check CMAKE_CXX_FLAGS, CMAKE_EXE_LINKER_FLAGS, and build-type specific flags
    set(MSAN_DETECTED FALSE)
    if(CMAKE_CXX_FLAGS MATCHES "-fsanitize=memory")
        set(MSAN_DETECTED TRUE)
    elseif(CMAKE_EXE_LINKER_FLAGS MATCHES "-fsanitize=memory")
        set(MSAN_DETECTED TRUE)
    elseif(CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE} MATCHES "-fsanitize=memory")
        set(MSAN_DETECTED TRUE)
    elseif(CMAKE_EXE_LINKER_FLAGS_${CMAKE_BUILD_TYPE} MATCHES "-fsanitize=memory")
        set(MSAN_DETECTED TRUE)
    endif()
    
    if(MSAN_DETECTED)
        # For MSan builds, manually add the test executable as a single test
        # because gtest_discover_tests fails due to MSan false positives during discovery
        # The test executable will still run all GoogleTest tests and report individual results
        message(STATUS "MSan detected: Using manual test registration instead of discovery")
        add_test(NAME disruptor_cpp_tests COMMAND disruptor_cpp_tests)
        set_tests_properties(disruptor_cpp_tests PROPERTIES TIMEOUT 300)
    else()
        # For non-MSan builds, use automatic test discovery
        gtest_discover_tests(disruptor_cpp_tests)
    endif()
else()
    message(STATUS "No ported Disruptor tests found under tests/disruptor/**")
endif()
