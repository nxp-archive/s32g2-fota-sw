
# stop the wifi connection

res=$(ps -aux | grep wpa_supplicant)
expect="wpa_supplicant -D"

if [[ $res == *$expect* ]]; then
    echo "[Info] The wpa_supplicant is running, try to stop it ..."
    pid=$(pgrep -f wpa_supplicant)
    kill -9 $pid
else
    echo "[Info] The wpa_supplicant is stopped."
fi

res=$(ps -aux | grep udhcpc)
expect="udhcpc -i wlan0"

if [[ $res == *$expect* ]]; then
    echo "[Info] The udhcpc is running, try to stop it ... "
    pid=$(pgrep -f udhcpc)
    kill -9 $pid
else
    echo "[Info] The udhcpc is stopped."
fi

