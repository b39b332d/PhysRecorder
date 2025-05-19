<br />
<div align="center">
  <a href="https://gitee.com/b39b332d/phy_recorder/">
    <img src="https://gitee.com/b39b332d/phy_recorder/raw/master/data/resources/icon.ico" alt="Logo" width="80" height="80">
  </a>

  <h3 align="center">PhysRecorder</h3>

  <p align="center">
    Physiological Signal & Realsense Recorder
    <br />
    <br />
    <a href="https://gitee.com/b39b332d/phy_recorder/issues">Report Bug</a>
    |
    <a href="https://gitee.com/b39b332d/phy_recorder/issues">Request Feature</a>
  </p>
</div>


<details>
  <summary>Table of Contents</summary>
  <ol>
    <li>
      <a href="#about-the-project">About The Project</a>
      <ul>
        <li><a href="#built-with">Built With</a></li>
      </ul>
    </li>
    <li>
      <a href="#getting-started">Getting Started</a>
      <ul>
        <li><a href="#prerequisites">Prerequisites</a></li>
        <li><a href="#installation">Installation</a></li>
      </ul>
    </li>
    <li><a href="#usage">Usage</a></li>
    <li><a href="#roadmap">Roadmap</a></li>
    <li><a href="#contributing">Contributing</a></li>
    <li><a href="#license">License</a></li>
    <li><a href="#contact">Contact</a></li>
  </ol>
</details>



<!-- ABOUT THE PROJECT -->
## About The Project

[![Product Screenshot][product-screenshot]](https://gitee.com/b39b332d/phy_recorder/)

**PhysRecorder** is a cross-platform tool for recording physiological signals and video streams (including RealSense cameras) with precise timestamp synchronization. It is designed to standardize experimental data collection, ensure reproducibility, and simplify the workflow for researchers and engineers.

支持的编码器: MJPG,HuffYUV,Raw
支持的摄像头驱动: MSMF, Realsense

该软件的编写目的：
- 让大家在进行实验时有标准的保存格式和录制流程
- 拥有同步信号采集的时间戳
- 避免造轮子

<p align="right">(<a href="#readme-top">back to top</a>)</p>


### Features

- Record video and multiple physiological signals (PPG, respiration, serial data) with synchronized timestamps.
- Support for various video encoders: MJPG, HuffYUV, Raw.
- Support for multiple camera drivers: MSMF, RealSense, V4L2.
- Output data in standardized formats for easy analysis.
- Cross-platform build support (Windows and Linux).
- Modern C++20 codebase, CMake build system, Ninja generator support.


### Built With

* C++
* CMake
* [Qt 6](https://www.qt.io/)
* [OpenCV 4](https://opencv.org/) With [OpenVINO](https://storage.openvinotoolkit.org/repositories/openvino/packages/2023.1/windows) DL
* [RealSense SDK 2](https://www.intelrealsense.com/sdk-2/)

<p align="right">(<a href="#readme-top">back to top</a>)</p>



<!-- GETTING STARTED -->
## Getting Started

You can choose Download Dependencies from Conda or build them yourself. The following instructions will get you a copy of the project up and running on your local machine for development and testing purposes.
### Prerequisites

#### Build Dependencies
* OpenCV
  ```Powershell
    cmake -G "Visual Studio 17 2022" -A "x64" `
		-DBUILD_JAVA=OFF -DBUILD_opencv_python2=OFF -DBUILD_opencv_python3=OFF -DBUILD_FAT_JAVA_LIB=OFF `
		-DMKL_WITH_OPENMP=ON -DMKL_WITH_TBB=ON `
		-DWITH_IPP=ON -DBUILD_WITH_DYNAMIC_IPP=ON `
		-DOPENCV_IPP_ENABLE_ALL=ON -DBUILD_WITH_DYNAMIC_IPP=ON `
		-DOPENCV_DNN_OPENVINO=ON -DWITH_OPENVINO=ON `
		-DOpenVINO_DIR="w_openvino_toolkit_windows_2024.5.0.17288.7975fa5da0c_x86_64\runtime\cmake" `
		-DMKL_ROOT_DIR="oneAPI\mkl\2024.2" `
		-DENABLE_CXX11=ON `
		-DCPU_BASELINE=AVX2 `
		-DBUILD_PERF_TESTS=OFF `
		-DBUILD_TESTS=OFF `
		-DBUILD_opencv_world=OFF `
		-DWITH_OPENGL=ON -DWITH_TBB=ON -DWITH_VTK=OFF ../
    cmake --build ./ --config Release
    cmake --install ./ --config Release --prefix ./install/release
    cmake --build ./ --config Debug
    cmake --install ./ --config Debug --prefix ./install/debug
  ```

#### Download Dependencies from Conda

```bash
apt install cmake gcc g++ v4l2-utils
conda create -n qt_env 
conda activate qt_env
conda install -c conda-forge librealsense libopencv=4.10.0  qt6-main pkg-config tbb-devel libopenvino mkl-devel cxx-compiler=1.7.0 libopengl
```
### Build

编译测试通过：
- MSVC(Visual Studio 2022) Windows x86-64
- MINGW 13.1.0 Windows x86-64
- G++ 11.4.0 Linux x86-64/aarch64

1. Clone the repo
   ```bash
   git clone https://gitee.com/b39b332d/phy_recorder.git
   ```
3. Generate Project
   ```bash
   mkdir ./build
   cd ./build
   cmake -DCMAKE_BUILD_TYPE=Release ../
   ```
4. build target PhysRecorder
   ```bash
   cmake --build ./ --config Release --target PhysRecorder
   ```

5. generate PhysRecorder exe
   ```bash
   cmake --install ./
   cmake --build ./ --target generate_boxed_exe
   ```

<p align="right">(<a href="#readme-top">back to top</a>)</p>

<!-- USAGE EXAMPLES -->
## Usage


录制文件在工作路径的rec目录下的录制时刻UNIX时间戳文件夹内

| 文件名        | File Content           | Format  |
| ------------- |:-------------:| -----:|
| `vid.avi`      | Video File | Video |
| `vid_ts.npy`      | Video Timestamp | UNIX timestamp (s) |
| `ppg_sig.npy`    | PPG Signal      |   uint16_t |
| `ppg_ts.npy` | PPG Timestamp      |    UNIX timestamp (s) |
| `respi_sig.npy` | Breath Signal      |    uint8_t |
| `respi_ts.npy` | Breath Timestamp      |    UNIX timestamp (s) |
| `seral_sig.npy` | Serial Signal      |    uint32_t |
| `seral_ts.npy` | Serial Timestamp      |   UNIX timestamp (s) |

- PPG and respiration signals are from Huake devices.
- Custom serial signal format: unsigned integer string, separated by `\r\n`.

其中PPG与呼吸信号为华科设备，自定义的串口信号读入格式为无符号整数的字符串格式，换行符号`\r\n`

<p align="right">(<a href="#readme-top">back to top</a>)</p>



<!-- ROADMAP -->
## Roadmap

- [x] Add README.md
- [ ] Add Comments
- [ ] add libcamera support
- [x] add clang support

See the [open issues](https://gitee.com/b39b332d/phy_recorder/issues) for a full list of proposed features (and known issues).

<p align="right">(<a href="#readme-top">back to top</a>)</p>



<!-- CONTRIBUTING -->
## Contributing

Contributions are what make the open source community such an amazing place to learn, inspire, and create. Any contributions you make are **greatly appreciated**.

If you have a suggestion that would make this better, please fork the repo and create a pull request. You can also simply open an issue with the tag "enhancement".
Don't forget to give the project a star! Thanks again!

按照该流程提交代码更新

1. Fork the Project
2. Create your Feature Branch (`git checkout -b feature/AmazingFeature`)
3. Commit your Changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the Branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

<p align="right">(<a href="#readme-top">back to top</a>)</p>



<!-- LICENSE -->
## License

Distributed under the MIT License. See `LICENSE.txt` for more information.

<p align="right">(<a href="#readme-top">back to top</a>)</p>



<!-- CONTACT -->
## Contact

[NBSLab](https://nbslab.njust.edu.cn/) - [Xingyan](https://www.b39b332d.cn/) - 122104010655@njust.edu.cn - 

Project Link: https://gitee.com/b39b332d/phy_recorder

<p align="right">(<a href="#readme-top">back to top</a>)</p>



[product-screenshot]: https://gitee.com/b39b332d/phy_recorder/raw/dev/data/resources/mainwindow.png