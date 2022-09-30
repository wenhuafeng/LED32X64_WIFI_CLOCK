#!/usr/bin/env python

import sys
import os
import datetime
import platform
import subprocess
import shutil

CHECKSUM = 'checksum.exe CLOCK_STM32F103C8T6_WIFI.hex CRC32 SHA256'

# mdk build
mdk_build_command = 'D:/Keil_v5/UV4/UV4.exe -j0 -r ./MDK-ARM/CLOCK_STM32F103C8T6_WIFI.uvprojx -o build_log.txt'
mdk_build_log     = 'cat ./MDK-ARM/build_log.txt'

# build out file
gcc_source_file_hex = 'build/CLOCK_STM32F103C8T6_WIFI.hex'
gcc_source_file_bin = 'build/CLOCK_STM32F103C8T6_WIFI.bin'
mdk_source_file_hex = 'MDK-ARM/CLOCK_STM32F103C8T6_WIFI/CLOCK_STM32F103C8T6_WIFI.hex'
mdk_source_file_bin = 'MDK-ARM/CLOCK_STM32F103C8T6_WIFI/CLOCK_STM32F103C8T6_WIFI.bin'
target_path = 'user/output'

# openocd define
stlink_config_file = "user/openocd/stlink.cfg"
chip_config_file = "user/openocd/stm32f1x.cfg"
program_cmd = "\"program user/output/CLOCK_STM32F103C8T6_WIFI.bin 0x8000000\""

# JLink file define
jlink_loadfile = 'user/output/CLOCK_STM32F103C8T6_WIFI.hex'
device = "STM32F103C8"
interface = "SWD"
speed = "1000"
address = "0x8000000"
jlink_cmdfile = ""

def stlink_download():
    host_os = platform.system()
    print("host os: %s" % host_os)

    if host_os == 'Windows':
        openocd_exe = "openocd.exe"
    elif host_os == 'Linux':
        openocd_exe = "openocd"
    else:
        print("unsupport platform")
        sys.exit(2)

    exec_cmd = "%s -f %s -f %s -c init -c halt -c %s -c reset -c shutdown\n" % \
               (openocd_exe, stlink_config_file, chip_config_file, program_cmd)
    print("execute cmd: %s" % exec_cmd)
    subprocess.call(exec_cmd, shell=True)

def gen_jlink_cmdfile(loadfile):
    global address, jlink_cmdfile

    dirname = os.path.dirname(os.path.abspath(loadfile))
    jlink_cmdfile = os.path.join(dirname, "%s.jlink" % device)
    print("gen jlink commands file: %s" % jlink_cmdfile)

    try:
        f = open(jlink_cmdfile, 'w')
        f.write("si 1\n")
        f.write("speed 4000\n")
        f.write("device STM32F103C8T6\n")
        f.write("r\n")
        f.write("h\n")
        f.write("erase\n")
        f.write("loadfile %s %s\n" % (loadfile, address))
        f.write("q\n")
    except IOError:
        print(IOError)  # will print something like "option -a not recognized"
        sys.exit(2)

def jlink_download(loadfile):
    global device, interface, speed, jlink_cmdfile

    host_os = platform.system()
    print("host os: %s" % host_os)
    gen_jlink_cmdfile(loadfile)

    if host_os == 'Windows':
        jlink_exe = "JLink.exe"
    elif host_os == 'Linux':
        jlink_exe = "JLinkExe"
    elif host_os == 'Darwin':
        jlink_exe = "JLinkExe"
    else:
        print("unsupport platform")
        sys.exit(2)

    exec_cmd = "%s -device %s -if %s -speed %s -CommanderScript %s" % \
               (jlink_exe, device, interface, speed, jlink_cmdfile)
    print("execute cmd: %s" % exec_cmd)
    subprocess.call(exec_cmd, shell=True)

def cp_build_file(source, target):
    assert not os.path.isabs(source)

    try:
        shutil.copy(source, target)
    except IOError as e:
        print("Unable to copy file. %s" % e)
    except:
        print("Unexpected error:", sys.exc_info())

def gcc_build():
    print("cc type = gcc")
    os.system('make clean')
    os.system('make -j8')
    cp_build_file(gcc_source_file_hex, target_path)
    cp_build_file(gcc_source_file_bin, target_path)

def mdk_build():
    print("cc type = mdk")
    os.system(mdk_build_command)
    os.system(mdk_build_log)
    cp_build_file(mdk_source_file_hex, target_path)
    cp_build_file(mdk_source_file_bin, target_path)

def build_select(para):
    if para == 'gcc':
        gcc_build()
    elif para == 'mdk':
        mdk_build()
    elif para == 'jlink':
        jlink_download(jlink_loadfile)
    elif para == 'stlink':
        stlink_download()
    else:
        print("input parameter error!")

def checksum_hex_file():
    # 切换目录到
    os.chdir(target_path)
    # 输出文件需要的信息
    os.system(CHECKSUM)

def main_func(para):
    start = datetime.datetime.now()

    build_select(para)
    if para == 'gcc' or 'mdk':
        print("\r\n")
        checksum_hex_file()

    end = datetime.datetime.now()
    print('run time: %s second' %(end - start))

if __name__ == "__main__":
    main_func(sys.argv[1])
