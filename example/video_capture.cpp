#include "video_capture.h"

VideoCapture::VideoCapture() {

}

VideoCapture::~VideoCapture() {

}

bool VideoCapture::StartCapture(const VideoDevice& video_device, const VideoDescription& video_description) {
	return true;
}

bool VideoCapture::StopCapture() {
	return true;
}

void VideoCapture::RegisterVideoFrameCallback(VideoFrameCallback callback) {
	callback_ = callback;
}