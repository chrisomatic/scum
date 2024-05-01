#!/bin/sh
rm -rf bin
mkdir bin

cd src

debug_arg=""
release_arg=""

if ! [ -z "$1" ]
then
    if [ "$1" == "debug" ]
    then
        debug_arg="-g"
        echo "building debug"
    elif [ "$1" == "release" ]
    then
        release_arg="-02"
        echo "building release"
    else
        echo "invalid arg"
    fi
fi

gcc core/gfx.c \
    core/shader.c \
    core/timer.c \
    core/math2d.c \
    core/window.c \
    core/imgui.c \
    core/glist.c \
    core/text_list.c \
    core/socket.c \
    core/particles.c \
    core/camera.c \
    core/lighting.c \
    core/bitpack.c \
    core/astar.c \
    core/audio.c \
    physics.c \
    effects.c \
    projectile.c \
    player.c \
    net.c \
    creature.c \
    level.c \
    editor.c \
    entity.c \
    item.c \
    explosion.c \
    status_effects.c \
    room_editor.c \
    room_file.c \
    skills.c \
    settings.c \
    ui.c \
    weapon.c \
    decal.c \
    gaudio.c \
    main.c \
    -Icore \
    -lglfw -lGLU -lGLEW -lGL -lopenal -lm $debug_arg $release_arg \
    -o ../bin/scum

    # build release
    #-lglfw -lGLU -lGLEW -lGL -lAL -lm -O2 \
