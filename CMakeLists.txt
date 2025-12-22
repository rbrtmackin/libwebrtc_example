cmake_minimum_required(VERSION 3.15)
project(WebRTCServerWithWebClient)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set this to your WebRTC root directory
# Download from: https://github.com/bengreenier/webrtc/releases/latest
set(WEBRTC_ROOT "C:/webrtc-prebuilt" CACHE PATH "Path to prebuilt WebRTC from bengreenier")

# Include directories
include_directories(
    ${WEBRTC_ROOT}/include
    ${WEBRTC_ROOT}/include/third_party/abseil-cpp
)

# Source files for the server
set(SERVER_SOURCES
    webrtc_server_http.cpp
    video_source.cpp
    throughput_receiver.cpp
    peer_connection_handler.cpp
)

# Header files
set(SERVER_HEADERS
    video_source.h
    throughput_receiver.h
    peer_connection_handler.h
    simple_video_factories.h
)

# Create server executable
add_executable(webrtc_server ${SERVER_SOURCES} ${SERVER_HEADERS})

# Windows-specific settings
if(WIN32)
    # Runtime library
    # bengreenier/webrtc uses /MT (static runtime), not /MD (dynamic)
    set_property(TARGET webrtc_server PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

    # Compiler options
    target_compile_options(webrtc_server PRIVATE
        /std:c++20          # Use C++20 standard
        /W3
        /wd4100  # Unreferenced formal parameter
        /wd4244  # Conversion possible loss of data
        /wd4267  # Conversion size_t to int
        /wd4458  # Declaration hides class member
    )

    # Preprocessor definitions
    target_compile_definitions(webrtc_server PRIVATE
        WEBRTC_WIN
        NOMINMAX
        WIN32_LEAN_AND_MEAN
        _WINSOCKAPI_
        RTC_ENABLE_WIN_WGC
        $<$<CONFIG:Debug>:_ITERATOR_DEBUG_LEVEL=0>
    )

    # Link against WebRTC
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(WEBRTC_LIB "${WEBRTC_ROOT}/debug/webrtc.lib")
    else()
        set(WEBRTC_LIB "${WEBRTC_ROOT}/release/webrtc.lib")
    endif()

    # Link all required libraries
    target_link_libraries(webrtc_server
        ${WEBRTC_LIB}
        ws2_32.lib
        secur32.lib
        winmm.lib
        dmoguids.lib
        wmcodecdspuuid.lib
        msdmo.lib
        strmiids.lib
        iphlpapi.lib
        crypt32.lib
        ole32.lib
        oleaut32.lib
        uuid.lib
    )
endif()

# Linux-specific settings
if(UNIX AND NOT APPLE)
    target_compile_definitions(webrtc_server PRIVATE
        WEBRTC_POSIX
        WEBRTC_LINUX
    )
    
    target_link_libraries(webrtc_server
        ${WEBRTC_ROOT}/lib/libwebrtc.a
        pthread
        dl
        rt
    )
endif()

# macOS-specific settings
if(APPLE)
    target_compile_definitions(webrtc_server PRIVATE
        WEBRTC_POSIX
        WEBRTC_MAC
    )
    
    target_link_libraries(webrtc_server
        ${WEBRTC_ROOT}/lib/libwebrtc.a
        pthread
    )
endif()

# Output directories
set_target_properties(webrtc_server PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Display build information
message(STATUS "============================================")
message(STATUS "WebRTC C++ HTTP Server with libwebrtc")
message(STATUS "============================================")
message(STATUS "WebRTC Root: ${WEBRTC_ROOT}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Output: ${CMAKE_BINARY_DIR}/bin/webrtc_server")
message(STATUS "Server Type: HTTP (port 9090)")
message(STATUS "Use with: Node.js signaling relay")
message(STATUS "============================================")

# Installation
install(TARGETS webrtc_server RUNTIME DESTINATION bin)
