#!/bin/sh
rm -rf bin
mkdir bin

cd src

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
    effects.c \
    player.c \
    level.c \
    main.c \
    -Icore \
    -lglfw -lGLU -lGLEW -lGL -lm \
    -o ../bin/scum

    # build release
    #-lglfw -lGLU -lGLEW -lGL -lm -O2 \
