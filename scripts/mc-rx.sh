#!/bin/sh
MCAST="$1"
PORT="$2"
#socat UDP4-RECVFROM:"$PORT",ip-add-membership="$MCAST":0.0.0.0,fork -
socat UDP4-RECVFROM:"$PORT",ip-add-membership="$MCAST":0.0.0.0 -

