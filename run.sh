#!/bin/bash

make

# キーボードデバイスのイベントファイルを検出
keyboard_event=$(awk '/keyboard/ {print; while (getline && $0 !~ /^$/) print}' /proc/bus/input/devices | grep -o 'event[0-9]\+')

if [ -z "$keyboard_event" ]; then
    echo "No keyboard device found."
    exit 1
fi

# プログラムを実行
sudo ./MyVirtualKeyboard "/dev/input/$keyboard_event"
