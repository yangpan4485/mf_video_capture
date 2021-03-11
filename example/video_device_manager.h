#pragma once

#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>

#include <vector>

#include "video_frame.h"

class VideoDeviceManager {
public:
	static VideoDeviceManager& Instance();

	std::vector<VideoDevice> GetAllVideoDevcies();
	std::vector<VideoDescription> GetVideoFormats(const VideoDevice& video_device);
	bool Init();

	IMFActivate* GetMFActive(const VideoDevice& video_device);
	IMFAttributes* GetMFAttrutes();
	GUID GetGuidByFormat(VideoType video_type);

private:
	VideoDeviceManager();
	~VideoDeviceManager();

	VideoDeviceManager(const VideoDeviceManager&) = delete;
	VideoDeviceManager operator =(const VideoDeviceManager&) = delete;

	void Clear();

private:
	IMFAttributes* attributes_{};
	IMFActivate** imf_active_{};
	UINT32 count_{};
	bool init_;
};