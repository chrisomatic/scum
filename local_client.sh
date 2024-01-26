#!/bin/sh

if [ -z "$1" ]
then
    args=""
else
    args="--name $1"
fi

./bin/scum --client 127.0.0.1 $args

