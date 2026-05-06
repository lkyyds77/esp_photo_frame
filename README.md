# ESP32-S3 WiFi 电子相框 (ESP Photo Frame)

## 简介
这是一个基于 ESP32-S3 芯片的电子相框项目。设备可以创建一个 Wi-Fi 热点，提供一个本地的 Web 图片服务器。用户可以通过网络上传图片并保存在 SD 卡中，同时在 LCD 屏幕上进行实时展示。

## 功能特性
- **Wi-Fi AP 模式**: 设备作为热点，方便移动设备或电脑直接连接组网 (`wifi_ap.c`)。
- **LCD 屏幕显示**: 驱动 LCD 屏幕展示相册图片 (`lcd_display.c`)。
- **SD 卡存储**: 文件系统挂载至 SD 卡，本地保存接收到的图片文件 (`sd_card.c`)。
- **Web 图像服务器**: 提供局域网内的 HTTP 服务，用于图片的上传和管理 (`image_server.c`)。
- **配置集中**: 统一的硬件接口和引脚配置 (`hardware_config.c`)。

## 硬件依赖
- ESP32-S3 开发板/核心板
- 兼容的 LCD 显示屏
- Micro SD/TF 卡槽模块
- *各模块引脚接线请参考 `hardware_config.h` 中的具体定义。*

## 软件框架
- 开发框架：[ESP-IDF](https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32s3/get-started/index.html)

## 目录结构
```text
├── CMakeLists.txt
├── main/
│   ├── CMakeLists.txt
│   ├── main.c              # 主程序入口
│   ├── hardware_config.c   # 硬件引脚及外设统一配置
│   ├── image_server.c      # Web 图片服务器及 HTTP 请求处理
│   ├── lcd_display.c       # LCD 屏幕驱动及显示逻辑
│   ├── sd_card.c           # SD 卡读写及 FATFS/VFS 挂载
│   └── wifi_ap.c           # Wi-Fi AP 模式网络配置
└── README.md
```

## 编译与烧录

确保已正确安装并配置 ESP-IDF 环境变量。

1. 清除旧配置并设定目标芯片 (仅初次编译或更换目标芯片时需要):
   ```bash
   idf.py set-target esp32s3
   ```
2. （可选）配置项目特定的设置参数:
   ```bash
   idf.py menuconfig
   ```
3. 编译、烧录并打开串口监视器:
   ```bash
   idf.py build flash monitor
   ```
*(注：如果需要指定串口端口，请使用 `-p` 参数，如 `idf.py -p COM3 flash monitor`)*

## 使用说明
1. **开机与连接**：给设备供电并开机，设备将启动并建立一个 Wi-Fi 热点。
2. **连接网络**：使用手机或电脑连接到相框释放的 Wi-Fi 信号（具体 SSID 及密码可在 `wifi_ap.c` 中配置）。
3. **上传照片**：在浏览器中访问相框所在的 IP 地址，进入 Web 管理页面。您可以通过该页面上传照片。
4. **展示照片**：上传的照片会被自动存储到 SD 卡中，接着相框会自动将其加载并在 LCD 屏幕上进行轮播。
