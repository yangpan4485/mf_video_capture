#pragma once
#include <wrl/client.h>

#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>

#include "video_capture.h"

class VideoCaptureReader : public VideoCapture, public IMFSourceReaderCallback {
public:
	VideoCaptureReader();
	~VideoCaptureReader();

	bool StartCapture(const VideoDevice& video_device, const VideoDescription& video_description) override;
	bool StopCapture() override;

private:
	HRESULT STDMETHODCALLTYPE OnReadSample(HRESULT hrStatus, DWORD dwStreamIndex, DWORD dwStreamFlags,
		LONGLONG llTimestamp, IMFSample *pSample) override;
	STDMETHODIMP QueryInterface(REFIID iid, void** ppv) override;
	STDMETHODIMP_(ULONG) AddRef() override;
	STDMETHODIMP_(ULONG) Release() override;
	STDMETHODIMP OnFlush(_In_  DWORD dwStreamIndex) override;
	STDMETHODIMP OnEvent(_In_  DWORD dwStreamIndex, _In_  IMFMediaEvent *pEvent) override;

private:
	IMFActivate* active_{};
	long ref_count_{};
	IMFSourceReader* source_reader_{};
};
