#include "video_capture_reader.h"
#include <Shlwapi.h>
#include <iostream>

#include "video_device_manager.h"

using Microsoft::WRL::ComPtr;

VideoCaptureReader::VideoCaptureReader() {

}

VideoCaptureReader::~VideoCaptureReader() {
	StopCapture();
}

bool VideoCaptureReader::StartCapture(const VideoDevice& video_device, 
	const VideoDescription& video_description) {
	// IMFAttributes* attributs = nullptr;
	ComPtr<IMFAttributes> attributs = nullptr;
	HRESULT hr = MFCreateAttributes(&attributs, 2);
	if (FAILED(hr)) {
		return false;
	}
	hr = attributs->SetUINT32(MF_READWRITE_DISABLE_CONVERTERS, TRUE);
	if (FAILED(hr)) {
		return false;
	}
	hr = attributs->SetUnknown(MF_SOURCE_READER_ASYNC_CALLBACK, (IUnknown*)this);
	if (FAILED(hr)) {
		return false;
	}

	ComPtr<IMFMediaSource> media_source = nullptr;
	// IMFMediaSource* media_source = nullptr;
	active_ = VideoDeviceManager::Instance().GetMFActive(video_device);
	if (!active_) {
		return false;
	}
	hr = active_->ActivateObject(__uuidof(IMFMediaSource), (void**)&media_source);
	if (FAILED(hr)) {
		return false;
	}

	hr = MFCreateSourceReaderFromMediaSource(media_source.Get(), attributs.Get(), &source_reader_);
	if (FAILED(hr)) {
		return false;
	}

	ComPtr<IMFMediaType> media_type;
	hr = source_reader_->GetNativeMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, &media_type);
	GUID guid = VideoDeviceManager::Instance().GetGuidByFormat(video_description.video_type);
	media_type->SetGUID(MF_MT_SUBTYPE, guid);
	MFSetAttributeSize(media_type.Get(), MF_MT_FRAME_SIZE, video_description.width, video_description.height);
	MFSetAttributeRatio(media_type.Get(), MF_MT_FRAME_RATE, video_description.fps, 1);
	hr = source_reader_->SetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, NULL, media_type.Get());
	if (FAILED(hr)) {
		return false;
	}
	hr = source_reader_->ReadSample(MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, NULL, NULL, NULL, NULL);
	if (FAILED(hr)) {
		return false;
	}
	return true;
}

bool VideoCaptureReader::StopCapture() {
	source_reader_ = nullptr;
	if (active_) {
		active_->ShutdownObject();
	}
	return true;
}

HRESULT STDMETHODCALLTYPE VideoCaptureReader::OnReadSample(HRESULT hrStatus, DWORD dwStreamIndex, DWORD dwStreamFlags,
	LONGLONG llTimestamp, IMFSample *pSample) {
	ComPtr<IMFMediaBuffer> buffer;
	HRESULT hr = S_OK;
	if (FAILED(hrStatus)) {
		hr = hrStatus;
	}
	if (SUCCEEDED(hr)) {
		if (pSample) {
			hr = pSample->GetBufferByIndex(0, &buffer);
			if (SUCCEEDED(hr)) {
				BYTE *data = NULL;
				DWORD size;
				buffer->GetCurrentLength(&size);
				hr = buffer->Lock(&data, NULL, NULL);
				buffer->Unlock();
				std::cout << "capture success" << std::endl;
			}
		}
	}
	if (SUCCEEDED(hr)) {
		hr = source_reader_->ReadSample((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM,
			0, NULL, NULL, NULL, NULL);
	}
	return hr;
}

STDMETHODIMP VideoCaptureReader::QueryInterface(REFIID iid, void** ppv) {
	static const QITAB qit[] = {
		QITABENT(VideoCaptureReader, IMFSourceReaderCallback),
		{ 0 },
	};
	return QISearch(this, qit, iid, ppv);
}

STDMETHODIMP_(ULONG) VideoCaptureReader::AddRef() {
	return InterlockedIncrement(&ref_count_);
}

STDMETHODIMP_(ULONG) VideoCaptureReader::Release() {
	ULONG count = InterlockedDecrement(&ref_count_);
	if (count == 0) {
		delete this;
	}
	return count;
}

STDMETHODIMP VideoCaptureReader::OnFlush(_In_  DWORD dwStreamIndex) {
	return S_OK;
}

STDMETHODIMP VideoCaptureReader::OnEvent(_In_  DWORD dwStreamIndex, _In_  IMFMediaEvent *pEvent) {
	return S_OK;
}
