#!/bin/sh
### BEGIN INIT INFO
# Provides: EnvSensorv2
# Required-Start:
# Required-Stop:
# Should-Start:
# Should-Stop:
# Default-Start: 2 3 4 5
# Default-Stop: 0 1 6
# Short-Description: Start and Stop
# Description:
### END INIT INFO

[ -r /etc/default/EnvSensorv2 ] && . /etc/default/EnvSensorv2


case "$START_SM" in
        [Nn]*)
                exit 0
                ;;
esac

case "$1" in
    start)
        echo "Starting EnvSensorv2"
        cd /home/pi/SNC2/SNCPython/EnvSensorv2
        python EnvSensorv2.py -c -d > /dev/null &2>1 &
        ;;

    stop)
        echo "Stopping EnvSensorv2"
        pkill -f -9 "python EnvSensorv2.py -c -d"
        ;;

    restart)
        echo "Stopping EnvSensorv2"
        pkill -f -9 "python EnvSensorv2.py -c -d"
        sleep 3
        echo "Starting EnvSensorv2"
        cd /home/pi/SNC2/SNCPython/EnvSensorv2
        python EnvSensorv2.py -c -d > /dev/null &2>1 &
        ;;

    *)
        echo "Usage: /etc/init.d/EnvSensorv2.sh {start|stop|restart}"
        exit 2
esac

