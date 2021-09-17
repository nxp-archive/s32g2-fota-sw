
# Enable wlan0
ifconfig wlan0 up

# Stop wifi before connect
sh stop_wifi.sh

con_file=wpa_supplicant.conf

if [ ! -f $con_file ];then
    echo "Not fount the $con_file file"
    exit 1
fi

wpa_supplicant -D wext -c $con_file -i wlan0 &

# Will block if not connect
udhcpc -i wlan0


echo "Wifi connection is OK!"
