#pragma once
#include <functional>

#include "video_frame.h"

class VideoCapture {
public:
	using VideoFrameCallback = std::function<void(VideoFrame& video_frame)>;

public:
	VideoCapture();
	virtual ~VideoCapture();

	virtual bool StartCapture(const VideoDevice& video_device, const VideoDescription& video_description);
	virtual bool StopCapture();

	void RegisterVideoFrameCallback(VideoFrameCallback callback);

protected:
	VideoFrameCallback callback_{};
};