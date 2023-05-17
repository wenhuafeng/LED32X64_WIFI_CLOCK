# 中文

### 一，功能说明：
  1，LED32X64 RGB三色LED点阵显示屏，HUB75D接口。<br/>
  2，STM32F103C8T6 MCU开发，关显示后RTC低功耗运行（非FreeRTOS版本才有此功能，FreeRTOS版本会一直运行）。<br/>
  3，时钟、万年历、星期、农历、温湿度、等。显示，显示亮度可以调节。<br/>
  4，人体红外感应点亮显示功能，5分钟无人自动关显示功能（如触发红外模块，重新开始显示5分钟）。<br/>
  5，点亮显示通过WiFi SNTP自动更新时间和日期，更新成功关闭WiFi电源。（通过宏开关打开此功能）<br/>
  6，可以通过GPS方式更新时间和日期，更新成功关闭GPS模块。（通过宏开关打开此功能）

### 二，编译说明：
  1，进入source目录。<br/>
  2，用MDK，命令行输入“python build.py m”开始编译，需要修改build.py内部MDK安装路途。<br/>
  3，用GCC，命令行输入“python build.py g”开始编译。<br/>

### 三，下载说明：
  1，进入source目录。<br/>
  2，用JLink，命令行输入“python build.py j”，开始下载固件到芯片。<br/>
  3，用ST-Link，需要用到openocd，命令行输入“python build.py stlink”，开始下载固件到芯片。


# English

### 1, Function description:
  1. LED32X64 RGB three color LED dot matrix display, HUB75D interface.<br/>
  2. STM32F103C8T6 MCU development. RTC runs with low power consumption after the display is turned off (this function is only available for non FreeRTOS versions, and FreeRTOS versions will always run).<br/>
  3. Clock, perpetual calendar, week, lunar calendar, temperature and humidity, etc. Display, display brightness can be adjusted.<br/>
  4. The human body infrared induction lights up the display function, and no one turns off the display function automatically for 5 minutes (if the infrared module is triggered, the display will restart for 5 minutes).<br/>
  5. When it is lit, the time and date are automatically updated through WiFi SNTP, and the WiFi power is turned off after the update. (This function is turned on through the macro switch)<br/>
  6. The time and date can be updated through GPS, and the GPS module is closed successfully. (This function is turned on through the macro switch)

### 2, Compilation description:
  1. Enter the source directory.<br/>
  2. With MDK, enter "python build.py m" on the command line to start compiling. You need to modify the internal MDK installation path of build.py.<br/>
  3. Use GCC and enter "python build.py g" on the command line to start compiling.

### 3, Download instructions:
  1. Enter the source directory.<br/>
  2. Use JLink and enter "python build.py j" on the command line to start downloading firmware to the chip.<br/>
  3. Use Stlink, you need to use openocd and enter "python build.py stlink" on the command line to start downloading firmware to the chip.
