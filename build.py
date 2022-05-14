#!/usr/bin/env python

import sys
import os
import datetime
import platform
import subprocess

mdk_build_command = 'D:/Keil_v5/UV4/UV4.exe -j0 -r ./MDK-ARM/CLOCK_STM32F103C8T6_WIFI.uvprojx -o build_log.txt'
mdk_build_log     = 'cat ./MDK-ARM/build_log.txt'
gcc_loadfile = 'build/CLOCK_STM32F103C8T6_WIFI.hex'
mdk_loadfile = 'MDK-ARM/CLOCK_STM32F103C8T6_WIFI/CLOCK_STM32F103C8T6_WIFI.hex'
device = "STM32F103C8"
interface = "SWD"
speed = "1000"
address = "0x8000000"
jlink_cmdfile = ""

def gen_jlink_cmdfile(loadfile):
    global address, jlink_cmdfile

    dirname = os.path.dirname(os.path.abspath(loadfile))
    jlink_cmdfile = os.path.join(dirname, "%s.jlink" % device)
    print("gen jlink commands file: %s" % jlink_cmdfile)

    try:
        f = open(jlink_cmdfile, 'w')
        f.write("h\n")
        f.write("loadfile %s %s\n" % (loadfile, address))
        f.write("g\n")
        f.write("r\n")
        f.write("qc")
    except IOError:
        print(IOError)  # will print something like "option -a not recognized"
        sys.exit(2)

def jlink_run(loadfile):
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

    exec_cmd = "%s -device %s -if %s -speed %s -CommanderScript %s" % (jlink_exe, device, interface, speed, jlink_cmdfile)
    print("execute cmd: %s" % exec_cmd)

    subprocess.call(exec_cmd, shell=True)

def gcc_build():
    os.system('make clean')
    os.system('make -j8')
    return

def mdk_build():
    os.system(mdk_build_command)
    os.system(mdk_build_log)
    return

def main_func(parameter):
    start = datetime.datetime.now()

    if parameter == 'gcc':
        print("cc type = gcc")
        gcc_build()
    elif parameter == 'mdk':
        print("cc type = MDK")
        mdk_build()
    elif parameter == 'gcc_bin_download':
        print("gcc bin download")
        jlink_run(gcc_loadfile)
    elif parameter == 'mdk_bin_download':
        print("mdk bin download")
        jlink_run(mdk_loadfile)
    else:
        print("input parameter error!")

    end = datetime.datetime.now()
    print('run time: %s second' %(end - start))
    return

def main():
    main_func(sys.argv[1])

if __name__ == "__main__":
    main()