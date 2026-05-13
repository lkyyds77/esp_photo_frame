# ESP32-S3 WiFi 电子相框 (ESP Photo Frame)

## 简介
这是一个基于 ESP32-S3 芯片的电子相框项目。设备可以创建一个 Wi-Fi 热点，提供一个本地的 Web 图片服务器。用户可以通过网络上传图片并保存在 SD 卡中，同时在 LCD 屏幕上进行实时展示。

## 功能特性
- **Wi-Fi AP 模式**: 设备作为热点，方便移动设备或电脑直接连接组网 (`wifi_ap.c`)。
- **LCD 屏幕显示**: 驱动 LCD 屏幕展示相册图片 (`lcd_display.c`)。
- **SD 卡存储**: 文件系统挂载至 SD 卡，本地保存接收到的图片文件 (`sd_card.c`)。
- **Web 图像服务器**: 提供局域网内的 HTTP 服务，用于图片的上传和管理 (`image_server.c`)。
- **共享 SPI 总线分时复用**: LCD 与 SD 共用同一 SPI Host，通过互斥保护实现“分时/CS 分时”访问，避免并发抢总线导致不稳定 (`app_context.c`)。
- **LCD DMA 刷新完成通知**: LCD 刷新走 SPI DMA 队列，并通过完成回调通知任务侧同步等待，减少 CPU 介入 (`lcd_display.c`)。
- **上传原子发布**: 上传写入临时文件后再重命名发布，避免轮播任务读取到“半写入”文件 (`image_server.c`)。
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
│   ├── app_context.c       # 全局事件组/互斥锁：任务协同与共享总线分时复用
│   ├── app_context.h
│   ├── main.c              # 主程序入口
│   ├── hardware_config.c   # 硬件引脚及外设统一配置
│   ├── image_server.c      # Web 图片服务器及 HTTP 请求处理
│   ├── lcd_display.c       # LCD 屏幕驱动及显示逻辑
│   ├── lcd_display.h
│   ├── sd_card.c           # SD 卡读写及 FATFS/VFS 挂载
│   ├── sd_card.h
│   └── wifi_ap.c           # Wi-Fi AP 模式网络配置
│   └── wifi_ap.h
└── README.md
```

## 架构与同步机制（概览）

本项目在“共享 SPI 总线（LCD + SD）”的硬件约束下，采用互斥与事件驱动来实现稳定并发：

- **共享资源互斥**：`g_shared_bus_mutex` 统一保护 SD 读写与 LCD 刷新，保证同一时刻只有一个外设占用 SPI 总线。
- **事件组协同**：`g_app_events` 用于标记各模块就绪与新图片到达等状态：
   - `APP_EVT_SD_READY` / `APP_EVT_LCD_READY` / `APP_EVT_WIFI_READY`：对应模块初始化完成
   - `APP_EVT_NEW_PHOTO`：上传完成后置位，轮播任务可提前唤醒切图

## 进度与待办（更新日期：2026-05-13）

### 已完成
- 现状梳理：确认原先瓶颈在 CPU 字节序交换、以及 SD/LCD 并发访问缺少协同。
- LCD DMA 刷新与同步：
   - 通过 SPI Panel IO 的完成回调通知任务侧“刷新完成”（用于等待 DMA 传输结束）。
   - 配置面板数据端序为 little-endian，移除逐像素 endian swap。
- 基础分时复用：用同一把互斥锁串行化 SD 读写与 LCD 刷新，提升共享总线稳定性。
- 上传可靠性：写临时文件再重命名发布，避免轮播读取半写入文件。
- 轮播任务化：显示逻辑从 `app_main()` 循环拆出为独立任务，并可在新图片到达时提前唤醒。

### 进行中（部分完成）
- FreeRTOS 双核流水线：目前已将显示任务固定在指定核心以降低与 Wi-Fi 栈的资源竞争，但“接收 → 缓存 → 写盘 → 通知显示”的完整队列化流水线尚未拆分完成。
- 更通用的 SPI/CS 分时机制：当前以互斥为主，尚未抽象成可扩展的事务队列/优先级仲裁层。

### 待完成（建议路线）
- 将 HTTP 上传从“回调内写 SD”解耦：回调只接收并投递到队列/环形缓冲，由专用写盘任务落盘。
- 构建完整事件驱动状态机：以事件组/队列打通 Wi-Fi 接收、SD 写入完成、LCD 刷新完成等关键节点。
- SPI 调度层增强：在互斥之上增加超时、统计与优先级（例如“写盘优先/刷新优先”的策略切换）。

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

### 备选构建方式（当 idf.py 环境不可用时）

在某些 Windows 环境中可能出现 ESP-IDF Python 环境不一致导致 `idf.py` 不可用。若你已经有可用的 `build/` 目录（即此前已完成过一次 CMake 配置），可以直接使用 Ninja 进行增量构建：

```bash
ninja -C build
```

说明：该方式用于“已有 build 目录”的增量构建；首次配置/切换 target 仍建议使用 `idf.py`。

## 使用说明
1. **开机与连接**：给设备供电并开机，设备将启动并建立一个 Wi-Fi 热点。
2. **连接网络**：使用手机或电脑连接到相框释放的 Wi-Fi 信号（具体 SSID 及密码可在 `wifi_ap.c` 中配置）。
3. **上传照片**：在浏览器中访问相框所在的 IP 地址，进入 Web 管理页面。您可以通过该页面上传照片。
4. **展示照片**：上传的照片会被自动存储到 SD 卡中，接着相框会自动将其加载并在 LCD 屏幕上进行轮播。
