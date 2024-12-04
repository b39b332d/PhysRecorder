#define UNICODE
#include <opencv2/imgcodecs.hpp>
#include <vector>
#include <chrono>
#include <iostream>
#include <thread>
#include <opencv2/imgproc.hpp>
#include <barrier>
#include <wincodecsdk.h>
#include <Windows.h>
#include <opencv2/highgui.hpp>
#include <mutex>
#include <Encoder.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
using namespace cv; 


// Create WIC factory
IWICImagingFactory* pFactory = nullptr;
std::mutex lock;

void encodeRGB888ToJPEG(uchar* rgbData, int width, int height, std::vector<unsigned char>& jpegBuffer)
{

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
	pEncoder->CreateNewFrame(&pFrameEncode, nullptr);
	pFrameEncode->Initialize(nullptr);

	// Set encoder properties
	pFrameEncode->SetSize(width, height);
	WICPixelFormatGUID pixelFormat = { GUID_WICPixelFormat24bppBGR };
	pFrameEncode->SetPixelFormat(&pixelFormat);
	pFrameEncode->WritePixels(height, width * 3, height * width * 3, reinterpret_cast<BYTE*>(const_cast<uchar*>(rgbData)));

	// Commit frame and encoder
	pFrameEncode->Commit();
	pEncoder->Commit();

	// Get JPEG data from memory stream
	STATSTG stat;
	pStream->Stat(&stat, STATFLAG_NONAME);
	ULONG jpegSize = static_cast<ULONG>(stat.cbSize.QuadPart);

	// Reset memory stream position
	LARGE_INTEGER zero = { 0 };
	pStream->Seek(zero, STREAM_SEEK_SET, nullptr);

	// Allocate buffer for JPEG data
	jpegBuffer.resize(jpegSize);

	// Read JPEG data into buffer
	pStream->Read(jpegBuffer.data(), jpegSize, nullptr);

	// Release resources
	pFrameEncode->Release();
	pEncoder->Release();
	pStream->Release();
}
int size_f = 0;

int stb_encode_jpg(uchar* rgbData, int width, int height,int comp,uchar* jpegBuffer) {
	//stbi__write_context s = { 0 };
	//uchar* temp = jpegBuffer;
	//s.context = &temp;
	//s.func = [](void* context, void* data, int size) {memcpy((*(uchar**)context), data, size); (*(uchar**)context) = (*(uchar**)context)+ size; };
	//stbi_write_jpg_core(&s, width, height, comp, rgbData, 90);
	//return (temp - jpegBuffer);
	return 1;
}
uchar jpegBuffer[1280 * 2 * 720 * 3];
int test_size = 100;
int main() {

	Mat img = imread("data/resources/face.jpg");
	cv::resize(img, img, Size(1280 * 2, 720));

	int size = stb_encode_jpg(img.data, 1280 * 2, 720, 3, jpegBuffer);

	int i;
	std::cin >> i;

}
/*
	{
		HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
		std::cout << "Encoder test\n";
		encoder::encoder_init();
		auto en_stream_p = encoder::add_stream(1280 * 2, 720,PIX_TYPE_BGR8);
		std::thread th([en_stream_p] {while (true) {
			auto out = encoder::read(en_stream_p);
			if (out == nullptr) {
				return;
			}
			else {
				size_f = out->size;
				delete out;
			}
		}});
		std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
		for (int i = 0; i < test_size; i++) {
			encoder::encode(std::shared_ptr<unsigned char>(img.data, [](auto p) {}),  en_stream_p);

		}
		encoder::stream_close_later(en_stream_p);
		th.join();
		std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
		std::cout << "Time difference = " << (double)std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() / 1000000 << "[s]" << std::endl;
		std::cout << size_f << std::endl;
		encoder::encoder_stop();
		//auto jpeg = encoder::read(en_stream_p);
		//Mat outimg = imdecode(jpeg, IMREAD_COLOR);
		//cv::imshow("", outimg);
		//waitKey(0);
	}
	{
		std::cout << "WIC multithread test\n";
		// Initialize COM
		CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFactory));
		static const int threads = CPU_COUNT;
		std::barrier bar(threads+1);
		std::vector<std::thread> th_pool;
		for (int i = 0; i < threads; i++)
			th_pool.emplace_back(
				std::thread([&bar, &img] {
					Mat imga;
					lock.lock();
					img.copyTo(imga);
					lock.unlock();
					bar.arrive_and_wait();

					for (int i = 0; i < test_size / threads; i++) {
						std::vector<uchar> buf;
						encodeRGB888ToJPEG(imga.data, imga.cols, imga.rows, buf);
						size_f = buf.size();
					}

					}));

		{
			bar.arrive_and_wait();
			std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
			for (auto& th : th_pool) {
				th.join();
			}
			std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
			std::cout << "Time difference = " << (double)std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() / 1000000 << "[s]" << std::endl;
			std::cout << size_f << std::endl;
		}


		// Uninitialize COM
		pFactory->Release();
		CoUninitialize();

	}
	//std::vector<uchar> buf_out;
	//encodeRGB888ToJPEG(img.data,img.cols,img.rows, buf_out);
	//Mat outimg = imdecode(buf_out, IMREAD_COLOR);
	//cv::imshow("", outimg);
	//waitKey(0);



	{
		std::cout << "opencv multithread test\n";
		static const int threads = CPU_COUNT;
		std::barrier bar(threads+1);
		std::vector<std::thread> th_pool;
		for (int i = 0; i < threads; i++)
			th_pool.emplace_back(
				std::thread([&bar, &img] {
					Mat imga;
					lock.lock();
					img.copyTo(imga);
					lock.unlock();
					bar.arrive_and_wait();

					for (int i = 0; i < test_size / threads; i++) {
						std::vector<uchar> buf;
						imencode(".png", img, buf);
						size_f = buf.size();
					}

					}));

		{
			int i;
			bar.arrive_and_wait();
			std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
			for (auto& th : th_pool) {
				th.join();
			}
			std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
			std::cout << "Time difference = " << (double)std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() / 1000000 << "[s]" << std::endl;
			std::cout << size_f << std::endl;
		}
	}

	{
		std::cout << "WIC single thread test\n";

		// Initialize COM
		HRESULT hr = CoInitialize(NULL);
		CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFactory));
		std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
		for (int i = 0; i < test_size; i++) {
			std::vector<uchar> buf;
			encodeRGB888ToJPEG(img.data, img.cols, img.rows, buf);
		}
		std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
		std::cout << "Time difference = " << (double)std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() / 1000000 << "[s]" << std::endl;


		pFactory->Release();
		CoUninitialize();
	}
	{

		std::cout << "Opencv single thread test\n";
		std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
		for (int i = 0; i < test_size; i++) {
			std::vector<uchar> buf;
			imencode(".jpg", img, buf);
		}
		std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
		std::cout << "Time difference = " << (double)std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() / 1000000 << "[s]" << std::endl;


	}
	return 0;
}
*/