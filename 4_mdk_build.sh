#!/bin/bash

function timediff() {
    # time format:date +"%s.%N", such as 1502758855.907197692
    start_time=$1
    end_time=$2

    start_s=${start_time%.*}
    start_nanos=${start_time#*.}
    end_s=${end_time%.*}
    end_nanos=${end_time#*.}

    # end_nanos > start_nanos?
    # Another way, the time part may start with 0, which means
    # it will be regarded as oct format, use "10#" to ensure
    # calculateing with decimal
    if [ "$end_nanos" -lt "$start_nanos" ];then
        end_s=$(( 10#$end_s - 1 ))
        end_nanos=$(( 10#$end_nanos + 10**9 ))
    fi

    # get timediff
    time=$(( 10#$end_s - 10#$start_s )).`printf "%03d\n" $(( (10#$end_nanos - 10#$start_nanos)/10**6 ))`

    echo $time"s"
}

start=$(date +"%s.%N")

#run function
echo Init building ...

"D:/Keil_v5/UV4/UV4.exe" -j0 -r "./MDK-ARM/CLOCK_STM32F103C8T6_WIFI.uvprojx" -o build_log.txt
cat ./MDK-ARM/build_log.txt

end=$(date +"%s.%N")

timediff $start $end

#echo Init building ...
#set UV=D:/Keil_v5/UV4/UV4.exe
#set UV_PRO_PATH=./MDK-ARM/CLOCK_STM32F103C8T6_WIFI.uvprojx
#%UV% -j0 -r %UV_PRO_PATH% -o %cd%./MDK-ARM/build_log.txt
#
#type MDK-ARM\build_log.txt
