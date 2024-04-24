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
    -lglfw -lGLU -lGLEW -lGL -lopenal -lm \
    -o ../bin/scum

    # build release
    #-lglfw -lGLU -lGLEW -lGL -lAL -lm -O2 \
