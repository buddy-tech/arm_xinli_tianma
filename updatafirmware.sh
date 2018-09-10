#!/bin/bash

pgClientPid()
{
    pgrep pgClient
}

if [ $# -lt 1 ]; then
    echo "error.. need args"
    exit 1
fi
echo "commond is $0"

clipid=`pgClientPid`
echo ${clipid}

kill ${clipid}
sleep 1

mount -t vfat /dev/mmcblk0p1 /tmp
#if [ $? -eq 0 ];
#then
#    exit 1
#if

for arg in "$@"
do
    echo $arg
    if [ "$arg" == "pgClient" ]; then
        cp -f /home/updata/pgClient /home/pgClient
        chmod 777 /home/pgClient
        sleep 1
    elif [ "$arg" == "7z030.bit" ]; then
        cp -f /home/updata/7z030.bit /tmp
        sync
    elif [ "$arg" == "BOOT.bin" ]; then
        cp -f /home/updata/BOOT.bin /tmp
        sync
    elif [ "$arg" == "uImage" ]; then
        cp -f /home/updata/uImage /tmp
        sync
    fi
done

sleep 1
reboot

