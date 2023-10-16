<br />
<div align="center">
  <a href="https://gitee.com/b39b332d/phy_recorder/">
    <img src="https://gitee.com/b39b332d/phy_recorder/raw/master/data/resources/icon.png" alt="Logo" width="80" height="80">
  </a>

  <h3 align="center">PhyRecorder</h3>

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

[![Product Name Screen Shot][product-screenshot]](https://gitee.com/b39b332d/phy_recorder/)

该软件编写时仅为完成个人实验要求，时间较早，代码并不规范，欢迎大家完善指正！

该软件的编写目的：
- 让大家在进行实验时有标准的保存格式和录制流程
- 拥有同步信号采集的时间戳
- 避免造轮子

<p align="right">(<a href="#readme-top">back to top</a>)</p>



### Built With

以下为该软件依赖的库以及构建工具

* C++
* CMake
* [Qt 6.3.1](https://www.qt.io/)
* [OpenCV 4.8.0](https://opencv.org/) With [OpenVINO 2023.0](https://storage.openvinotoolkit.org/repositories/openvino/packages/2023.1/windows) DL
* [RealSense SDK 2](https://www.intelrealsense.com/sdk-2/)

<p align="right">(<a href="#readme-top">back to top</a>)</p>



<!-- GETTING STARTED -->
## Getting Started

该软件目前构建仅支持Windows平台。

### Prerequisites

构建依赖库。

* OpenCV
  ```Powershell
    # Use Proxy
    $env:HTTP_PROXY = "http://127.0.0.1:28080"
    $env:HTTPS_PROXY = "http://127.0.0.1:28080"
    # Use MKL
    "C:\Program Files (x86)\Intel\oneAPI\setvars.bat"
    cmake -G "Visual Studio 17 2022" -A "x64" `
		-DBUILD_JAVA=OFF -DBUILD_opencv_python2=OFF -DBUILD_opencv_python3=OFF -DBUILD_FAT_JAVA_LIB=OFF `
		-DMKL_WITH_OPENMP=ON -DMKL_WITH_TBB=ON `
		-DOPENCV_DNN_OPENVINO=ON -DWITH_OPENVINO=ON`
		-DOpenVINO_DIR= <OPENVINO_PATH>`
		-DENABLE_CXX11=ON `
		-DCPU_BASELINE=AVX2 `
		-DBUILD_PERF_TESTS=OFF `
		-DBUILD_TESTS=OFF `
		-DVIDEOIO_PLUGIN_LIST=all `
		-DBUILD_opencv_world=OFF `
		-DWITH_LIBREALSENSE=ON `
		-DWITH_QT=ON `#Fancy imshow window
		-DLIBREALSENSE_LIBRARIES="C:/Program Files (x86)/Intel RealSense SDK 2.0/lib/x64" `
		-DLIBREALSENSE_INCLUDE_DIR="C:/Program Files (x86)/Intel RealSense SDK 2.0/include" `
		-DWITH_QT=ON -DWITH_OPENGL=ON -DWITH_TBB=ON ../
    cmake --build ./ --config Release
    cmake --install ./ --config Release --prefix ./install/release
    cmake --build ./ --config Debug
    cmake --install ./ --config Debug --prefix ./install/debug
  ```

### Installation

编译测试通过：MSVC(Visual Studio 2022) Windows x64

未经过测试：MINGW GNU

1. Clone the repo
   ```powershell
   git clone https://gitee.com/b39b332d/phy_recorder.git
   ```
3. Generate Project
   ```powershell
   mkdir ./build
   cd ./build
   cmake ../`
         -DOpenCV_BUILD_Path=/Path_to_OpenCV_Dir/`
         -DQt6_ROOT=/Path_to_Qt6_Dir/`
         -DREALSENSE_DIR=/Path_to_Realsense_Dir/`
         -DOpenVINO_DIR=/Path_to_OpenVino_Dir/`
         -DMKL_DIR=/Path_to_MKL_Dir/ # Use MKL`
   ```
4. build and install target PhyRecorder
   ```powershell
   cmake --build ./ --config Release --target PhyRecorder
   cmake --install ./ --config Release --target PhyRecorder
   ```

<p align="right">(<a href="#readme-top">back to top</a>)</p>



<!-- USAGE EXAMPLES -->
## Usage

建议使用[Enigma Virtual Box](https://enigmaprotector.com/en/aboutvb.html)打包。

注意，打包时请勿打包工作路径下的rec目录。

录制文件在工作路径的rec目录下的录制时刻UNIX时间戳文件夹内

| 文件名        | 文件内容           | 内容格式  |
| ------------- |:-------------:| -----:|
| `vid.avi`      | 视频文件 | MJPEG |
| `vid_ts.npy`      | 视频时间戳 | UNIX timestamp (s) |
| `ppg_sig.npy`    | PPG信号      |   uint16_t |
| `ppg_ts.npy` | PPG时间戳      |    UNIX timestamp (s) |
| `respi_sig.npy` | 呼吸信号      |    uint8_t |
| `respi_ts.npy` | 呼吸信号时间戳      |    UNIX timestamp (s) |
| `seral_sig.npy` | 串口信号      |    uint32_t |
| `seral_ts.npy` | 串口信号时间戳      |   UNIX timestamp (s) |

其中PPG与呼吸信号为华科设备，自定义的串口信号读入格式为无符号整数的字符串格式，换行符号`\r\n`

<p align="right">(<a href="#readme-top">back to top</a>)</p>



<!-- ROADMAP -->
## Roadmap

- [x] 添加 README.md
- [ ] 修改Bug以及完善代码
- [ ] 添加注释
- [ ] 增加 Linux 支持
- [ ] 增加 MINGW 支持
- [ ] 语言Resource提示支持
    - [*] English
    - [ ] Chinese

See the [open issues](https://github.com/othneildrew/Best-README-Template/issues) for a full list of proposed features (and known issues).

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

[NBSLab](https://nbslab-njust.feishu.cn/) - [Xingyan](https://www.b39b332d.cn/) - 122104010655@njust.edu.cn - 

Project Link: https://gitee.com/b39b332d/phy_recorder

<p align="right">(<a href="#readme-top">back to top</a>)</p>



[product-screenshot]: https://gitee.com/b39b332d/phy_recorder/raw/dev/data/resources/mainwindow.png