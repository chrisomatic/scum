#!/bin/sh

args=""

if ! [ -z "$1" ]
then
    if [ "$1" == "server" ]
    then
        args="--server"
    fi
fi

# ./build.sh debug && gdb -ex run ./bin/scum
./build.sh debug && gdb -ex=r --args ./bin/scum $args
