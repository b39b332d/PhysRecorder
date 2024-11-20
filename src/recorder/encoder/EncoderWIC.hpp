
#define UNICODE
#include <vector>
#include <iostream>
#include <EncoderComp.h>
#include <wincodecsdk.h>
#include <Windows.h>
#include <libyuv.h>
static int is_msvc_init = false;
namespace encoder {

	class EncoderWIC :public EncoderComp {
		IWICImagingFactory* pFactory = nullptr;
		std::mutex lock;
		float quality_wic ;
		PIX_TYPE e_type;
		WICJpegYCrCbSubsamplingOption sub_method;
	public:
		EncoderWIC(int width, int height, PIX_TYPE e_type, int quality);
		EncodedFrame* encode(RawFrame* rgbData);
		~EncoderWIC();
	};
	EncoderWIC::EncoderWIC(int width, int height, PIX_TYPE e_type, int quality): 
		EncoderComp(width, height, e_type, quality)
	, e_type(e_type){
		if (!is_msvc_init) {
			CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
			is_msvc_init = true;
		}
		CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFactory));
		
		// quality default -1, range [-1 ,0~99,100,101]
		// WIC map to	   0.9       [0.9,0~0.99,mathematically lossless,ml_without_subsample]
		if (quality <0)quality_wic = 0.9;
		else {
			if (quality == 100) quality_wic = 1;
			else if (quality > 100) {
				 quality = 100;
			}
			else
				quality_wic = (float)quality/100;
		}		


	}
	EncodedFrame* EncoderWIC::encode(RawFrame* rgbData)
	{
		if (e_type == PIX_TYPE_MJPG) {

			EncodedFrame* jpegBuffer = new EncodedFrame(rgbData->raw_frame, unsigned(rgbData->raw_frame_len),
				[rgbData]() {rgbData->release(); });
			return jpegBuffer;
		}

		// Create memory stream for JPEG output
		IStream* pStream = nullptr;

		CreateStreamOnHGlobal(nullptr, TRUE, &pStream);

		// Create JPEG encoder
		IWICBitmapEncoder* pEncoder = nullptr;
		lock.lock();
		pFactory->CreateEncoder(GUID_ContainerFormatJpeg, nullptr, &pEncoder);
		lock.unlock();
		pEncoder->Initialize(pStream, WICBitmapEncoderNoCache);

		// Create JPEG frame encoder
		IWICBitmapFrameEncode* pFrameEncode = nullptr;
		PROPBAG2 propBag[2] = { 0, };
		VARIANT varValue[2];
		IPropertyBag2* pPropertyBag = nullptr;
		pEncoder->CreateNewFrame(&pFrameEncode, &pPropertyBag);
		propBag[0].pstrName = const_cast <wchar_t*>(L"ImageQuality");
		propBag[0].dwType = PROPBAG2_TYPE_DATA;
		propBag[0].vt = VT_R4;
		varValue[0].vt = VT_R4;
		varValue[0].fltVal = quality_wic;
		propBag[1].pstrName = const_cast <wchar_t*>(L"JpegYCrCbSubsampling");
		propBag[1].dwType = PROPBAG2_TYPE_DATA;
		propBag[1].vt = VT_UI1;
		varValue[1].vt = VT_UI1;

		if (e_type == PIX_TYPE_YUY2 || e_type == PIX_TYPE_UYVY || e_type == PIX_TYPE_YUYV) {
			unsigned char* c[5];
			varValue[1].bVal = WICJpegYCrCbSubsampling422;
			IWICPlanarBitmapFrameEncode* planarFrame = nullptr;
			pPropertyBag->Write(2, propBag, varValue);
			pFrameEncode->Initialize(pPropertyBag);
			pPropertyBag->Release();
			pFrameEncode->SetSize(width, height);
			auto f = GUID_WICPixelFormat24bppBGR;
			pFrameEncode->SetPixelFormat(&f);
			auto hr = pFrameEncode->QueryInterface(IID_PPV_ARGS(&planarFrame));
			c[0] = (unsigned char*)malloc(width * height);
			c[1] = (unsigned char*)malloc(width * height / 2);
			c[2] = (unsigned char*)malloc(width * height / 2);
			if(e_type == PIX_TYPE_UYVY)
				libyuv::UYVYToI422(rgbData->raw_frame, width * 2,
					c[0], width, c[1], width / 2, c[2], width / 2, width, height);

			else
				libyuv::YUY2ToI422(rgbData->raw_frame, width * 2,
					c[0], width, c[1], width / 2, c[2], width / 2, width, height);
			rgbData->release();
			WICBitmapPlane planes[3];
			planes[0].Format = GUID_WICPixelFormat8bppY;
			planes[0].pbBuffer = c[0];
			planes[0].cbStride = width;
			planes[0].cbBufferSize = width*height;

			planes[1].Format = GUID_WICPixelFormat8bppCb;
			planes[1].pbBuffer = c[1];
			planes[1].cbStride = width/2;
			planes[1].cbBufferSize = width * height/2;

			planes[2].Format = GUID_WICPixelFormat8bppCr;
			planes[2].pbBuffer = c[2];
			planes[2].cbStride = width / 2;
			planes[2].cbBufferSize = width * height/2;
			hr = planarFrame->WritePixels(height, planes, 3);
			pFrameEncode->Commit();
			pEncoder->Commit();
			planarFrame->Release();

			free(c[0]);
			free(c[1]);
			free(c[2]);
		}
		else if (e_type == PIX_TYPE_NV12) {
			unsigned char* c[5];
			varValue[1].bVal = WICJpegYCrCbSubsampling420;
			IWICPlanarBitmapFrameEncode* planarFrame = nullptr;
			pPropertyBag->Write(2, propBag, varValue);
			pFrameEncode->Initialize(pPropertyBag);
			pPropertyBag->Release();
			pFrameEncode->SetSize(width, height);
			auto f = GUID_WICPixelFormat24bppBGR;
			pFrameEncode->SetPixelFormat(&f);
			auto hr = pFrameEncode->QueryInterface(IID_PPV_ARGS(&planarFrame));

			WICBitmapPlane planes[3];
			planes[0].Format = GUID_WICPixelFormat8bppY;
			planes[0].pbBuffer = rgbData->raw_frame;
			planes[0].cbStride = width;
			planes[0].cbBufferSize = width * height;

			planes[1].Format = GUID_WICPixelFormat16bppCbCr;
			planes[1].pbBuffer = rgbData->raw_frame+ width * height;
			planes[1].cbStride = width ;
			planes[1].cbBufferSize = height * width/2;

			hr = planarFrame->WritePixels(height, planes, 2);
			pFrameEncode->Commit();
			pEncoder->Commit();
			planarFrame->Release();
			rgbData->release();

		}
		else if (e_type == PIX_TYPE_NV21) {
			varValue[1].bVal = WICJpegYCrCbSubsampling420;
			IWICPlanarBitmapFrameEncode* planarFrame = nullptr;
			pPropertyBag->Write(2, propBag, varValue);
			pFrameEncode->Initialize(pPropertyBag);
			pPropertyBag->Release();
			pFrameEncode->SetSize(width, height);
			auto f = GUID_WICPixelFormat24bppBGR;
			pFrameEncode->SetPixelFormat(&f);
			auto hr = pFrameEncode->QueryInterface(IID_PPV_ARGS(&planarFrame));

			unsigned char* c = (unsigned char*)malloc(width*height/2);
			libyuv::SwapUVPlane(rgbData->raw_frame + width * height, width,
				c, width, width, height/2);

			WICBitmapPlane planes[3];
			planes[0].Format = GUID_WICPixelFormat8bppY;
			planes[0].pbBuffer = rgbData->raw_frame;
			planes[0].cbStride = width;
			planes[0].cbBufferSize = width * height;

			planes[1].Format = GUID_WICPixelFormat16bppCbCr;
			planes[1].pbBuffer = c;
			planes[1].cbStride = width;
			planes[1].cbBufferSize = height * width / 2;

			hr = planarFrame->WritePixels(height, planes, 2);
			pFrameEncode->Commit();
			pEncoder->Commit();
			planarFrame->Release();
			rgbData->release();
			free(c);

		}
		else if (e_type == PIX_TYPE_Y12I || e_type == PIX_TYPE_I420) {
			varValue[1].bVal = WICJpegYCrCbSubsampling420;
			IWICPlanarBitmapFrameEncode* planarFrame = nullptr;
			pPropertyBag->Write(2, propBag, varValue);
			pFrameEncode->Initialize(pPropertyBag);
			pPropertyBag->Release();
			pFrameEncode->SetSize(width, height);
			auto f = GUID_WICPixelFormat24bppBGR;
			pFrameEncode->SetPixelFormat(&f);
			auto hr = pFrameEncode->QueryInterface(IID_PPV_ARGS(&planarFrame));

			WICBitmapPlane planes[3];
			planes[0].Format = GUID_WICPixelFormat8bppY;
			planes[0].pbBuffer = rgbData->raw_frame;
			planes[0].cbStride = width;
			planes[0].cbBufferSize = width * height;

			planes[1].Format = GUID_WICPixelFormat8bppCb;
			planes[1].pbBuffer = rgbData->raw_frame + width * height;
			planes[1].cbStride = width/2;
			planes[1].cbBufferSize = height * width / 4;

			planes[1].Format = GUID_WICPixelFormat8bppCr;
			planes[1].pbBuffer = rgbData->raw_frame + int(width * height *1.25);
			planes[1].cbStride = width/2;
			planes[1].cbBufferSize = height * width / 4;

			hr = planarFrame->WritePixels(height, planes, 3);
			pFrameEncode->Commit();
			pEncoder->Commit();
			planarFrame->Release();
			rgbData->release();
		}
		else  {
			WICPixelFormatGUID pixelFormat;
			switch (e_type)
			{
			PIX_TYPE_RGB888:
				pixelFormat = { GUID_WICPixelFormat24bppRGB };
				break;
			PIX_TYPE_GRAY08:
				pixelFormat = { GUID_WICPixelFormat8bppGray };
				break;
			PIX_TYPE_GRAY16:
				pixelFormat = { GUID_WICPixelFormat16bppGray };
				break;
			PIX_TYPE_RGBA32:
				pixelFormat = { GUID_WICPixelFormat32bppRGBA };
				break;
			PIX_TYPE_BGRA32:
				pixelFormat = { GUID_WICPixelFormat32bppBGRA };
				break;
			default:
				pixelFormat = { GUID_WICPixelFormat24bppBGR };
				break;
			}
			varValue[1].bVal = WICJpegYCrCbSubsampling422;
			pPropertyBag->Write(2, propBag, varValue);
			pFrameEncode->Initialize(pPropertyBag);
			pPropertyBag->Release();

			pFrameEncode->SetSize(width, height);
			pFrameEncode->SetPixelFormat(&pixelFormat);
			pFrameEncode->WritePixels(height, width* comp, data_size, rgbData->raw_frame);
			pFrameEncode->Commit();
			pEncoder->Commit();
			rgbData->release();
		}

		// Get JPEG data from memory stream
		STATSTG stat;
		pStream->Stat(&stat, STATFLAG_NONAME);
		ULONG jpegSize = static_cast<ULONG>(stat.cbSize.QuadPart);

		// Reset memory stream position
		LARGE_INTEGER zero = { 0 };
		pStream->Seek(zero, STREAM_SEEK_SET, nullptr);

		// Allocate buffer for JPEG data
		auto jpg_temp = (unsigned char*)malloc(jpegSize);
		EncodedFrame* jpegBuffer = new EncodedFrame(jpg_temp, jpegSize );

		// Read JPEG data into buffer
		pStream->Read(jpg_temp, jpegSize, nullptr);

		// Release resources
		pFrameEncode->Release();
		pEncoder->Release();
		pStream->Release();
		return jpegBuffer;
	}

	EncoderWIC::~EncoderWIC() {

		pFactory->Release();
	}
};
