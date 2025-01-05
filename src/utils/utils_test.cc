#include <iostream>
#include <cstdlib>
#include <windows.h>
#include <string>
#include <vector>
#include <filesystem>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/dnn.hpp>
#include <opencv2/imgcodecs.hpp>

namespace fs = std::filesystem;

int main() {
    // Print all environment variables
    std::cout << "Environment Variables:\n";
    char* env = std::getenv("PATH");
    if (env) {
        std::cout << "PATH: " << env << std::endl;
    }

    // Get all environment variables and print them
    LPWCH lpEnv = GetEnvironmentStringsW();
    if (lpEnv != nullptr) {
        wchar_t* envVar = lpEnv;
        while (*envVar) {
            std::wcout << envVar << std::endl;
            envVar += wcslen(envVar) + 1;
        }
        FreeEnvironmentStringsW(lpEnv);
    }

    // Get and print the current directory using std::filesystem (C++17 and later)
    std::cout << "\nCurrent Directory: " << fs::current_path() << std::endl;

    // List files in the current directory
    std::cout << "\nFiles in the Current Directory:\n";
    try {
        for (const auto& entry : fs::directory_iterator(fs::current_path())) {
            std::cout << entry.path() << std::endl;
        }
    }
    catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Error listing files: " << e.what() << std::endl;
    }
    int i;
    std::cin >> i;

    {
        cv::Mat img(cv::Size(100, 100), CV_8UC3);
        cv::Mat img2(cv::Size(100, 100), CV_8UC3);
        cv::Mat o = img + img2;

        cv::dnn::Net net_face;
        cv::dnn::Net net_landmarks;
        std::string root_path = "./data/";
        std::string path_net_facedetect = root_path + "models/intel/face-detection-retail-0005/FP16/face-detection-retail-0005";
        std::string path_net_landmarks = root_path + "models/intel/facial-landmarks-35-adas-0002/FP16/facial-landmarks-35-adas-0002";
        std::string resource_path = root_path + "resources/";


        net_landmarks = cv::dnn::readNetFromModelOptimizer(path_net_landmarks + ".xml", path_net_landmarks + ".bin");
        net_landmarks.setPreferableBackend(cv::dnn::DNN_BACKEND_INFERENCE_ENGINE);
        net_landmarks.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
        cv::Mat test_face = cv::imread(resource_path + "face.jpg");
        net_landmarks.setInput(cv::dnn::blobFromImage(test_face, 1, cv::Size(60, 60)));
        auto w = net_landmarks.forwardAsync();
    }
    return 0;
}