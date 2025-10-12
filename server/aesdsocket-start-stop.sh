#!/bin/sh

DAEMON=/usr/bin/aesdsocket
OPTIONS="-d"

case "$1" in
start)
  echo "Starting daemon $DAEMON"
  start-stop-daemon -S -n aesdsocket -a $DAEMON -- $OPTIONS
  ;;
stop)
  echo "Stopping daemon $DAEMON"
  start-stop-daemon -K -n aesdsocket
  ;;
*)
  exit 1
  ;;
esac

exit 0
