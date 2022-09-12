功能说明：
1，LED32X64 RGB三色LED点阵显示屏，HUB75D接口。
2，STM32F103C8T6 MCU开发，关显示后RTC低功耗运行（非FreeRTOS版本才有此功能，FreeRTOS版本会一直运行）。
3，时钟、万年历、星期、农历、温湿度、等。显示，显示亮度可以调节。
4，人体红外感应点亮显示功能，5分钟无人自动关显示功能（如触发红外模块，重新开始显示5分钟）。
5，点亮显示通过WiFi SNTP自动更新时间和日期，更新成功关闭WiFi电源。（通过宏开关打开此功能）
6，可以通过GPS方式更新时间和日期，更新成功关闭GPS模块。（通过宏开关打开此功能）

编译说明：
1，用MDK，命令行输入“python build.py mdk”开始编译，需要修改build.py内部MDK安装路途。
2，用GCC，命令行输入“python build.py gcc”开始编译。

下载说明：
1，用JLink，命令行输入“python build.py jlink”，开始下载固件到芯片。
2，用ST-Link，命令行输入“python build.py stlink”，开始下载固件到芯片。