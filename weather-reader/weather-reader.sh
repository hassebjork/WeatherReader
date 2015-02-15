#! /bin/sh
#http://www.tldp.org/HOWTO/HighQuality-Apps-HOWTO/boot.html
### BEGIN INIT INFO
# Provides:          weather-reader
# Required-Start:    $network $time $remote_fs
# Required-Stop:     $network $time $remote_fs
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Short-Description: Deferred execution scheduler
# Description:       Debian init script for the weather-reader deferred executions
#                    scheduler
### END INIT INFO
#
# Author:	Hasse Bjork
#

PATH=/bin:/usr/bin
DAEMON=/usr/bin/weather-reader
PIDFILE=/var/run/weather-reader.pid

test -x $DAEMON || exit 0

. /lib/lsb/init-functions

case "$1" in
  start)
	log_daemon_msg "Starting Weather Reader" "weather-reader"
	start_daemon -p $PIDFILE $DAEMON
	log_end_msg $?
    ;;
  stop)
	log_daemon_msg "Stopping Weather Reader" "weather-reader"
	killproc -p $PIDFILE $DAEMON
	log_end_msg $?
    ;;
  force-reload|restart)
    $0 stop
    $0 start
    ;;
  status)
    status_of_proc -p $PIDFILE $DAEMON weather-reader && exit 0 || exit $?
    ;;
  *)
    echo "Usage: /etc/init.d/weather-reader {start|stop|restart|force-reload|status}"
    exit 1
    ;;
esac

exit 0
