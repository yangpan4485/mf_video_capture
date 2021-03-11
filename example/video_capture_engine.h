#pragma once
#include <mfcaptureengine.h>
#include <strsafe.h>
#include <commctrl.h>
#include <d3d11.h>
#include <wrl/client.h>

#include "video_capture.h"

class MFVideoCallback;

class VideoCaptureEngine : public VideoCapture {
public:
	VideoCaptureEngine();
	~VideoCaptureEngine();

	bool StartCapture(const VideoDevice& video_device, const VideoDescription& video_description) override;
	bool StopCapture() override;

	void OnEvent(IMFMediaEvent* media_event);
private:
	bool InitCaptureEngine(const VideoDevice& video_device);
	bool CreateD3DManager();
	bool GetAvailableIndex(IMFCaptureSource* source, int& stream_index, int& media_type_index, const VideoDescription& video_description);
	HRESULT WaitOnCaptureEvent(GUID capture_event_guid);

private:
	Microsoft::WRL::ComPtr<IMFCaptureEngine> capture_engine_{};
	MFVideoCallback* video_callback_{};

	bool is_initialized_{};
	bool is_started_{};

	HANDLE error_handle_{};
	HANDLE initial_handle_{};

	Microsoft::WRL::ComPtr<IMFMediaSource> source_{};
	Microsoft::WRL::ComPtr<IMFDXGIDeviceManager> dxgi_device_manager_{};
	Microsoft::WRL::ComPtr<ID3D11Device> dx11_device_{};
	UINT reset_token_{};
};
