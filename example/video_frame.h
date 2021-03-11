#pragma once
#include <string>

#define RELEASE_AND_CLEAR(p) \
    if (p) {                 \
        (p)->Release();      \
        (p) = NULL;          \
    }

enum VideoType {
	kVideoTypeUnknown,
	kVideoTypeI420,
	kVideoTypeIYUV,
	kVideoTypeRGB24,
	kVideoTypeABGR,
	kVideoTypeARGB,
	kVideoTypeARGB4444,
	kVideoTypeRGB565,
	kVideoTypeARGB1555,
	kVideoTypeYUY2,
	kVideoTypeYV12,
	kVideoTypeUYVY,
	kVideoTypeMJPEG,
	kVideoTypeNV21,
	kVideoTypeNV12,
	kVideoTypeBGRA,
};

struct VideoDevice {
	uint32_t index{};
	std::string device_name{};
	std::string device_id{};
};

struct VideoDescription {
	uint32_t width{};
	uint32_t height{};
	uint32_t fps{};
	VideoType video_type{};
};

struct VideoFrame {
	uint8_t* y_data;
	uint32_t y_stride;
	uint8_t* u_data;
	uint32_t u_stride;
	uint8_t* v_data;
	uint32_t v_stride;
	uint32_t width;
	uint32_t height;
	VideoType video_type{};
};