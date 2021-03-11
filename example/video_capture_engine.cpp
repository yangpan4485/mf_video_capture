#include "video_capture_engine.h"

#include <mfapi.h>
#include <mferror.h>
#include <stddef.h>
#include <wincodec.h>
#include <strmif.h>
#include <iostream>

#include "video_device_manager.h"

#pragma comment(lib, "D3D11.lib")

using Microsoft::WRL::ComPtr;

class MFVideoCallback : public IMFCaptureEngineOnSampleCallback, public IMFCaptureEngineOnEventCallback {
public:
	MFVideoCallback(VideoCaptureEngine* observer) : observer_(observer) {}
	~MFVideoCallback() {}
	IFACEMETHODIMP QueryInterface(REFIID riid, void** object) override {
		HRESULT hr = E_NOINTERFACE;
		if (riid == IID_IUnknown) {
			*object = this;
			hr = S_OK;
		}
		else if (riid == IID_IMFCaptureEngineOnSampleCallback) {
			*object = static_cast<IMFCaptureEngineOnSampleCallback*>(this);
			hr = S_OK;
		}
		else if (riid == IID_IMFCaptureEngineOnEventCallback) {
			*object = static_cast<IMFCaptureEngineOnEventCallback*>(this);
			hr = S_OK;
		}
		if (SUCCEEDED(hr))
			AddRef();

		return hr;
	}

	IFACEMETHODIMP_(ULONG) AddRef() override {
		return InterlockedIncrement(&m_cRef);
	}

	IFACEMETHODIMP_(ULONG) Release() override {
		LONG cRef = InterlockedDecrement(&m_cRef);
		if (cRef == 0)
		{
			delete this;
		}
		return cRef;
	}

	IFACEMETHODIMP OnEvent(IMFMediaEvent* media_event) override {
		if (!observer_) {
			return S_OK;
		}
		observer_->OnEvent(media_event);
		return S_OK;
	}

	IFACEMETHODIMP OnSample(IMFSample* sample) override {
		// std::cout << "OnSample" << std::endl;
		return S_OK;
	}

	void Shutdown() {
		observer_ = nullptr;
	}

private:
	// Protects access to |observer_|.
	VideoCaptureEngine* observer_;
	long m_cRef;
};

HRESULT CreateDX11Device(_Out_ ID3D11Device** ppDevice, _Out_ ID3D11DeviceContext** ppDeviceContext, _Out_ D3D_FEATURE_LEVEL* pFeatureLevel)
{
	HRESULT hr = S_OK;
	static const D3D_FEATURE_LEVEL levels[] = {
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3,
		D3D_FEATURE_LEVEL_9_2,
		D3D_FEATURE_LEVEL_9_1
	};


	hr = D3D11CreateDevice(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		D3D11_CREATE_DEVICE_VIDEO_SUPPORT,
		levels,
		ARRAYSIZE(levels),
		D3D11_SDK_VERSION,
		ppDevice,
		pFeatureLevel,
		ppDeviceContext
	);

	if (SUCCEEDED(hr))
	{
		ID3D10Multithread* pMultithread;
		hr = ((*ppDevice)->QueryInterface(IID_PPV_ARGS(&pMultithread)));

		if (SUCCEEDED(hr))
		{
			pMultithread->SetMultithreadProtected(TRUE);
		}

		RELEASE_AND_CLEAR(pMultithread);

	}

	return hr;
}

HRESULT CopyAttribute(IMFAttributes* source_attributes, IMFAttributes* destination_attributes, const GUID& key) {
	PROPVARIANT var;
	PropVariantInit(&var);
	HRESULT hr = source_attributes->GetItem(key, &var);
	if (FAILED(hr)) {
		return hr;
	}

	hr = destination_attributes->SetItem(key, var);
	PropVariantClear(&var);
	return hr;
}

HRESULT GetMFSinkMediaSubtype(IMFMediaType* source_media_type, bool use_hardware_format, GUID* mf_sink_media_subtype, bool* passthrough) {
	GUID source_subtype;
	HRESULT hr = source_media_type->GetGUID(MF_MT_SUBTYPE, &source_subtype);
	if (FAILED(hr)) {
		return hr;
	}
	*mf_sink_media_subtype = MFVideoFormat_NV12;
	*passthrough = (MFVideoFormat_NV12 == source_subtype);
	return S_OK;
}

HRESULT ConvertToVideoSinkMediaType(IMFMediaType* source_media_type, IMFMediaType* sink_media_type, GUID type) {
	HRESULT hr = sink_media_type->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
	if (FAILED(hr)) {
		return hr;
	}

	hr = sink_media_type->SetGUID(MF_MT_SUBTYPE, type);
	if (FAILED(hr)) {
		return hr;
	}

	hr = CopyAttribute(source_media_type, sink_media_type, MF_MT_FRAME_SIZE);
	if (FAILED(hr)) {
		return hr;
	}

	hr = CopyAttribute(source_media_type, sink_media_type, MF_MT_FRAME_RATE);
	if (FAILED(hr)) {
		return hr;
	}

	hr = CopyAttribute(source_media_type, sink_media_type, MF_MT_PIXEL_ASPECT_RATIO);
	if (FAILED(hr)) {
		return hr;
	}

	return CopyAttribute(source_media_type, sink_media_type, MF_MT_INTERLACE_MODE);
}

VideoCaptureEngine::VideoCaptureEngine() {
	initial_handle_ = CreateEvent(NULL, FALSE, FALSE, NULL);
	error_handle_ = CreateEvent(NULL, FALSE, FALSE, NULL);
}

VideoCaptureEngine::~VideoCaptureEngine() {
	StopCapture();
	if (video_callback_) {
		delete video_callback_;
		video_callback_ = nullptr;
	}
}

bool VideoCaptureEngine::StartCapture(const VideoDevice& video_device, const VideoDescription& video_description) {
	if (!is_initialized_) {
		if (InitCaptureEngine(video_device)) {
			is_initialized_ = true;
		}
	}
	if (!is_initialized_) {
		return false;
	}

	ComPtr<IMFCaptureSource> source;
	HRESULT hr = capture_engine_->GetSource(&source);
	if (FAILED(hr)) {
		return false;
	}

	int stream_index = 0;
	int media_type_index = 0;
	GetAvailableIndex(source.Get(), stream_index, media_type_index, video_description);
	
	ComPtr<IMFMediaType> source_video_media_type;
	source->GetAvailableDeviceMediaType(stream_index, media_type_index, &source_video_media_type);
	
	hr = source->SetCurrentDeviceMediaType(stream_index, source_video_media_type.Get());
	if (FAILED(hr)) {
		return false;
	}
	

	ComPtr<IMFCaptureSink> sink;
	hr = capture_engine_->GetSink(MF_CAPTURE_ENGINE_SINK_TYPE_PREVIEW, &sink);
	if (FAILED(hr)) {
		return false;
	}
	
	ComPtr<IMFCapturePreviewSink> preview_sink;
	hr = sink->QueryInterface(IID_PPV_ARGS(&preview_sink));
	if (FAILED(hr)) {
		return false;
	}
	hr = preview_sink->RemoveAllStreams();
	if (FAILED(hr)) {
		return false;
	}
	ComPtr<IMFMediaType> sink_video_media_type;
	hr = MFCreateMediaType(&sink_video_media_type);
	if (FAILED(hr)) {
		return false;
	}

	GUID type = VideoDeviceManager::Instance().GetGuidByFormat(video_description.video_type);
	hr = ConvertToVideoSinkMediaType(source_video_media_type.Get(), sink_video_media_type.Get(), type);
	if (FAILED(hr)) {
		return false;
	}

	DWORD dw_sink_stream_index = 0;
	hr = preview_sink->AddStream(stream_index, sink_video_media_type.Get(), nullptr, &dw_sink_stream_index);
	if (FAILED(hr)) {
		return false;
	}
	hr = preview_sink->SetSampleCallback(dw_sink_stream_index, video_callback_);
	if (FAILED(hr)) {
		return false;
	}
	
	hr = capture_engine_->StartPreview();
	if (FAILED(hr)) {
		return false;
	}
	is_started_ = true;
	return true;
}

bool VideoCaptureEngine::StopCapture() {
	if (is_started_ && capture_engine_) {
		capture_engine_->StopPreview();
	}
	is_started_ = false;
	return true;
}

void VideoCaptureEngine::OnEvent(IMFMediaEvent* media_event) {
	HRESULT hr;
	GUID capture_event_guid = GUID_NULL;
	media_event->GetStatus(&hr);
	media_event->GetExtendedType(&capture_event_guid);
	if (capture_event_guid == MF_CAPTURE_ENGINE_ERROR || FAILED(hr)) {
		SetEvent(error_handle_);
		// There should always be a valid error
		hr = SUCCEEDED(hr) ? E_UNEXPECTED : hr;
	}
	else if (capture_event_guid == MF_CAPTURE_ENGINE_INITIALIZED) {
		SetEvent(initial_handle_);
	}
}

bool VideoCaptureEngine::InitCaptureEngine(const VideoDevice& video_device) {
	ComPtr<IMFCaptureEngineClassFactory> capture_engine_class_factory;
	HRESULT hr = CoCreateInstance(CLSID_MFCaptureEngineClassFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&capture_engine_class_factory));
	if (FAILED(hr)) {
		return false;
	}
	hr = capture_engine_class_factory->CreateInstance(CLSID_MFCaptureEngine, IID_PPV_ARGS(&capture_engine_));
	if (FAILED(hr)) {
		return false;
	}
	ComPtr<IMFAttributes> attributes;
	hr = MFCreateAttributes(&attributes, 1);
	if (FAILED(hr)) {
		return false;
	}
	hr = attributes->SetUINT32(MF_CAPTURE_ENGINE_USE_VIDEO_DEVICE_ONLY, TRUE);
	if (FAILED(hr)) {
		return false;
	}
	if (CreateD3DManager()) {
		return false;
	}
	hr = attributes->SetUnknown(MF_CAPTURE_ENGINE_D3D_MANAGER, dxgi_device_manager_.Get());
	if (FAILED(hr)) {
		return false;
	}

	video_callback_ = new MFVideoCallback(this);
	hr = capture_engine_->Initialize(video_callback_, attributes.Get(), nullptr, VideoDeviceManager::Instance().GetMFActive(video_device));
	if (FAILED(hr)) {
		return false;
	}
	WaitOnCaptureEvent(MF_CAPTURE_ENGINE_INITIALIZED);
	return true;
}

bool VideoCaptureEngine::CreateD3DManager() {
	HRESULT hr = S_OK;
	D3D_FEATURE_LEVEL FeatureLevel;
	ID3D11DeviceContext* pDX11DeviceContext;

	hr = CreateDX11Device(&dx11_device_, &pDX11DeviceContext, &FeatureLevel);

	if (SUCCEEDED(hr))
	{
		hr = MFCreateDXGIDeviceManager(&reset_token_, &dxgi_device_manager_);
	}

	if (SUCCEEDED(hr))
	{
		hr = dxgi_device_manager_->ResetDevice(dx11_device_.Get(), reset_token_);
	}

	RELEASE_AND_CLEAR(pDX11DeviceContext);

	return hr;
}

bool VideoCaptureEngine::GetAvailableIndex(IMFCaptureSource* source, int& stream_index, int& media_type_index, const VideoDescription& video_description) {
	DWORD count = 0;
	HRESULT hr = source->GetDeviceStreamCount(&count);
	for (DWORD index = 0; index < count; index++) {
		MF_CAPTURE_ENGINE_STREAM_CATEGORY stream_category;
		hr = source->GetDeviceStreamCategory(index, &stream_category);
		if (FAILED(hr)) {
			return false;
		}
		DWORD type_index = 0;
		ComPtr<IMFMediaType> type;
		while (SUCCEEDED(hr = source->GetAvailableDeviceMediaType(index, type_index, &type))) {
			GUID major_type_guid;
			type->GetGUID(MF_MT_MAJOR_TYPE, &major_type_guid);
			GUID sub_type_guid;
			type->GetGUID(MF_MT_SUBTYPE, &sub_type_guid);
			UINT32 width32, height32;
			MFGetAttributeSize(type.Get(), MF_MT_FRAME_SIZE, &width32, &height32);
			UINT32 frame_rate = 0u;
			UINT32 denominator = 0u;
			MFGetAttributeRatio(type.Get(), MF_MT_FRAME_RATE, &frame_rate, &denominator);
			if (width32 == video_description.width && height32 == video_description.height && 
				sub_type_guid == VideoDeviceManager::Instance().GetGuidByFormat(video_description.video_type)
				&& frame_rate == video_description.fps) {
				stream_index = index;
				media_type_index = type_index;
				break;
			}
			type.Reset();
			++type_index;
		}
	}
	if (hr == MF_E_NO_MORE_TYPES) {
		hr = S_OK;
	}
	if (FAILED(hr)) {
		return false;
	}
	return true;
}

HRESULT VideoCaptureEngine::WaitOnCaptureEvent(GUID capture_event_guid) {
	HRESULT hr = S_OK;
	HANDLE events[] = { initial_handle_, error_handle_ };
	DWORD wait_result = ::WaitForMultipleObjects(2, events, FALSE, INFINITE);
	switch (wait_result) {
	case WAIT_OBJECT_0:
		break;
	case WAIT_FAILED:
		hr = HRESULT_FROM_WIN32(::GetLastError());
		break;
	default:
		hr = E_UNEXPECTED;
		break;
	}
	return hr;
}