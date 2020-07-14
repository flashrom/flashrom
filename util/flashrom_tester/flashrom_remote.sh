#!/bin/sh
/usr/bin/ssh localhost -p 60024 -C /usr/sbin/flashrom "$@"
