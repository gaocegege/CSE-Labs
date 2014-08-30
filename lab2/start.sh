#!/usr/bin/env bash

ulimit -c unlimited

YFSDIR1=$PWD/yfs1

rm -rf $YFSDIR1
mkdir $YFSDIR1 || exit 1
sleep 1
echo "starting ./yfs_client $YFSDIR1  > yfs_client1.log 2>&1 &"
./yfs_client $YFSDIR1   > yfs_client1.log 2>&1 &

sleep 2

# make sure FUSE is mounted where we expect
pwd=`pwd -P`
if [ `mount | grep "$pwd/yfs1" | grep -v grep | wc -l` -ne 1 ]; then
    sh stop.sh
    echo "Failed to mount YFS properly at ./yfs1"
    exit -1
fi
