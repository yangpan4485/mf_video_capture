#include <iostream>
#include <vector>

#include "video_device_manager.h"
#include "string_utils.h"
#include "video_capture_engine.h"
#include "video_capture_reader.h"
#include "video_capture.h"

int main(void) {
	auto devices = VideoDeviceManager::Instance().GetAllVideoDevcies();
	std::cout << "devices size : " << devices.size() << std::endl;
	for (size_t i = 0; i < devices.size(); ++i) {
		std::cout << utils::Utf8ToAnsi(devices[i].device_name) << std::endl;
	}
	auto formats = VideoDeviceManager::Instance().GetVideoFormats(devices[0]);
	VideoDescription format;
	std::cout << "format size:" << formats.size() << std::endl;
	for (size_t i = 0; i < formats.size(); ++i) {
		std::cout << "width:" << formats[i].width << " height:" << formats[i].height << std::endl;
		if (640 == formats[i].width && 480 == formats[i].height && formats[i].video_type == kVideoTypeNV12) {
			format = formats[i];
			break;
		}
	}
#if 1
	VideoCaptureEngine capture;
	capture.StartCapture(devices[1], format);
	getchar();
	capture.StopCapture();
#else if
	VideoCaptureReader video_capture;
	video_capture.StartCapture(devices[0], format);
	getchar();
	video_capture.StopCapture();
#endif
	
	return 0;
}