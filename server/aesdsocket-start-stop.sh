#!/bin/bash

DAEMON=/usr/bin/aesdsocket
OPTIONS="-d"

case $1 in
start)
  echo "Starting daemon $DAEMON"
  start-stop-daemon --start --quiet --pidfile /var/run/$DAEMON.pid --make-pidfile --background --exec $DAEMON $OPTIONS
  ;;
stop)
  echo "Stopping daemon $DAEMON"
  start-stop-daemon --stop --quiet --pidfile /var/run/$DAEMON.pid
  ;;
esac

exit 0
