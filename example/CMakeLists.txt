set(DEMO_SOURCE
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/video_device_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/video_device_manager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/video_frame.h
    ${CMAKE_CURRENT_SOURCE_DIR}/string_utils.h
    ${CMAKE_CURRENT_SOURCE_DIR}/string_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/video_capture.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/video_capture.h
    ${CMAKE_CURRENT_SOURCE_DIR}/video_capture_engine.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/video_capture_engine.h
    ${CMAKE_CURRENT_SOURCE_DIR}/video_capture_reader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/video_capture_reader.h
    )

add_executable(mf_demo ${DEMO_SOURCE})

target_link_libraries(mf_demo mfplat mf mfreadwrite mfuuid d3d9 shlwapi)

set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT mf_demo)