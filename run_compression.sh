#!/bin/bash

if [ -z "$1" ]; then
    echo "Usage: $0 <path_to_movie>";
    exit 1;
fi

MOVIE=$1

command -v ffmpeg >/dev/null 2>&1 || { echo "ffmpeg not found! Aborting."; exit 1; }

DIR_NAME="/tmp/koda_compression.`basename $MOVIE`.$$.tmp"
mkdir "$DIR_NAME"

FRAME_DIGITS=`strings $MOVIE | grep FRAME | wc -l | wc -m`

ffmpeg -y -i "$1" -qscale:v 0 "$DIR_NAME/%0${FRAME_DIGITS}d.png" >/dev/null 2>&1

./koda_compression "$DIR_NAME"

rm -rf "$DIR_NAME"

