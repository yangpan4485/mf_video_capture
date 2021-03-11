#include "video_device_manager.h"

#include <iostream>
#include <string>
#include <atlbase.h>

#include "video_frame.h"
#include "string_utils.h"

#pragma comment(lib,"mfplat.lib")
#pragma comment(lib,"mf.lib")
#pragma comment(lib,"mfreadwrite.lib")
#pragma comment(lib,"mfuuid.lib")
#pragma comment(lib,"d3d9.lib")
#pragma comment(lib,"shlwapi.lib")

VideoDeviceManager& VideoDeviceManager::Instance() {
	static VideoDeviceManager instance;
	return instance;
}

VideoDeviceManager::VideoDeviceManager() {
	Init();
}

VideoDeviceManager::~VideoDeviceManager() {
	Clear();
	RELEASE_AND_CLEAR(attributes_);
}

void VideoDeviceManager::Clear() {
	for (UINT32 i = 0; i < count_; i++)
	{
		RELEASE_AND_CLEAR(imf_active_[i]);
	}
	CoTaskMemFree(imf_active_);
	imf_active_ = NULL;
	count_ = 0;
}

bool VideoDeviceManager::Init() {
	if (init_) {
		return true;
	}
	HRESULT hr = S_OK;
	hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

	if (SUCCEEDED(hr)) {
		hr = MFStartup(MF_VERSION);
	}
	if (FAILED(hr)) {
		return false;
	}
	init_ = true;
	return true;
}

IMFActivate* VideoDeviceManager::GetMFActive(const VideoDevice& video_device) {
	return imf_active_[video_device.index];
}

IMFAttributes* VideoDeviceManager::GetMFAttrutes() {
	return attributes_;
}

std::vector<VideoDevice> VideoDeviceManager::GetAllVideoDevcies() {
	Clear();
	std::vector<VideoDevice> result;
	HRESULT hr = S_OK;
	if (!attributes_) {
		hr = MFCreateAttributes(&attributes_, 1);
		if (FAILED(hr)) {
			return result;
		}
	}
	hr = attributes_->SetGUID(
		MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
		MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID
	);
	if (FAILED(hr)) {
		return result;
	}
	
	hr = MFEnumDeviceSources(attributes_, &imf_active_, &count_);
	if (FAILED(hr)) {
		return result;
	}
	
	for (UINT32 i = 0; i < count_; ++i) {
		WCHAR* device_name = NULL;
		WCHAR* device_id = NULL;
		VideoDevice video_device;
		hr = imf_active_[i]->GetAllocatedString(
			MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME,
			&device_name,
			NULL
		);

		hr = imf_active_[i]->GetAllocatedString(
			MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK,
			&device_id,
			NULL
		);
		
		video_device.device_id = utils::UnicodeToUtf8(device_id);
		video_device.device_name = utils::UnicodeToUtf8(device_name);
		video_device.index = i;
		result.push_back(std::move(video_device));
		CoTaskMemFree(device_name);
		CoTaskMemFree(device_id);
	}
	return result;
}

std::vector<VideoDescription> VideoDeviceManager::GetVideoFormats(const VideoDevice& video_device) {
	std::vector<VideoDescription> video_descriptions_{};
	if (!init_) {
		return video_descriptions_;
	}
	IMFMediaSource* source = NULL;
	HRESULT hr = imf_active_[video_device.index]->ActivateObject(__uuidof(IMFMediaSource), (void**)&source);
	if (FAILED(hr)) {
		return video_descriptions_;
	}

	CComPtr<IMFPresentationDescriptor> pd = nullptr;
	CComPtr<IMFStreamDescriptor> sd = nullptr;
	CComPtr<IMFMediaTypeHandler> handle = nullptr;

	BOOL bSelected = false;
	unsigned long types = 0;
	hr = source->CreatePresentationDescriptor(&pd);
	if (FAILED(hr)) {
		return video_descriptions_;
	}
	hr = pd->GetStreamDescriptorByIndex(0, &bSelected, &sd);
	if (FAILED(hr)) {
		return video_descriptions_;
	}
	hr = sd->GetMediaTypeHandler(&handle);
	if (FAILED(hr)) {
		return video_descriptions_;
	}
	hr = handle->GetMediaTypeCount(&types);
	if (FAILED(hr)) {
		return video_descriptions_;
	}
	
	for (int i = 0; i < types; i++) {
		CComPtr<IMFMediaType> type = nullptr;
		hr = handle->GetMediaTypeByIndex(i, &type);
		if (FAILED(hr)) {
			continue;
		}
		VideoDescription video_desc;

		UINT32 frameRate = 0u;
		UINT32 denominator = 0u;
		MFGetAttributeRatio(type, MF_MT_FRAME_RATE, &frameRate, &denominator);
		
		if (frameRate > 30 || frameRate < 10 || (frameRate % 5 != 0)) {
			continue;
		}
		video_desc.fps = frameRate;

		UINT32 width, height;
		MFGetAttributeSize(type, MF_MT_FRAME_SIZE, &width, &height);
		if (width % 10 != 0 || height % 10 != 0 || width < 320 || height < 240 || 
			width > 1280 || height > 720) {
			continue;
		}
		video_desc.width = width;
		video_desc.height = height;

		GUID subtype = { 0 };
		hr = type->GetGUID(MF_MT_SUBTYPE, &subtype);
		if (MFVideoFormat_NV12 != subtype && MFVideoFormat_MJPG != subtype) {
			continue;
		}
		if (MFVideoFormat_NV12 == subtype) {
			video_desc.video_type = kVideoTypeNV12;
		}
		else if (MFVideoFormat_MJPG == subtype) {
			video_desc.video_type = kVideoTypeMJPEG;
		}

		video_descriptions_.push_back(std::move(video_desc));
	}
	
	return video_descriptions_;
}

GUID VideoDeviceManager::GetGuidByFormat(VideoType video_type) {
	switch (video_type) {
	case kVideoTypeNV12:
		return MFVideoFormat_NV12;
	case kVideoTypeMJPEG:
		return MFVideoFormat_MJPG;
	default:
		break;
	}
	return MFVideoFormat_Base;
}