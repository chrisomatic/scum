@echo off

echo Removing old build files
rmdir /S /Q .\build .\bin

echo Creating new build directories
mkdir .\build
mkdir .\bin
pushd .\build

set srcdir=..\..\src
set bindir=..\bin

echo Copying data files to output folder
rmdir /Q /S src
mkdir %bindir%\src\core\shaders %bindir%\src\core\fonts %bindir%\src\img %bindir%\src\effects %bindir%\src\themes %bindir%\src\rooms

xcopy %srcdir%\img %bindir%\src\img
xcopy %srcdir%\effects %bindir%\src\effects
xcopy %srcdir%\themes %bindir%\src\themes
xcopy %srcdir%\rooms %bindir%\src\rooms
xcopy ..\dlls\*.dll %bindir%

xcopy %srcdir%\core\shaders %bindir%\src\core\shaders
xcopy %srcdir%\core\fonts %bindir%\src\core\fonts

set srcfiles=%srcdir%\core\gfx.c %srcdir%\core\shader.c %srcdir%\core\timer.c %srcdir%\core\math2d.c %srcdir%\core\window.c %srcdir%\core\astar.c %srcdir%\core\imgui.c %srcdir%\core\glist.c %srcdir%\core\text_list.c %srcdir%\core\socket.c %srcdir%\core\particles.c %srcdir%\core\camera.c %srcdir%\core\lighting.c %srcdir%\core\bitpack.c %srcdir%\physics.c %srcdir%\effects.c %srcdir%\projectile.c %srcdir%\player.c %srcdir%\net.c %srcdir%\decal.c %srcdir%\creature.c %srcdir%\level.c %srcdir%\editor.c %srcdir%\entity.c %srcdir%\item.c %srcdir%\explosion.c %srcdir%\status_effects.c %srcdir%\room_editor.c %srcdir%\room_file.c %srcdir%\skills.c %srcdir%\settings.c %srcdir%\ui.c %srcdir%\weapon.c %srcdir%\main.c

set opts=/O2 /TC /D "_CRT_SECURE_NO_WARNINGS" /nologo
set includes=/I..\include /I%srcdir% /I%srcdir%\core /I..\dlls
set libs="OpenGL32.lib" "GLu32.lib" "glfw3_mt.lib" "glew32.lib" "kernel32.lib" "user32.lib" "gdi32.lib" "winspool.lib" "comdlg32.lib" "advapi32.lib" "shell32.lib" "ole32.lib" "oleaut32.lib" "uuid.lib" "odbc32.lib" "odbccp32.lib"

echo Compiling project
echo cl %opts% %includes% %srcfiles% /link /LIBPATH:..\lib /NODEFAULTLIB:MSVCRT %libs% /OUT:%bindir%\Scum.exe 
cl %opts% %includes% %srcfiles% /link /LIBPATH:..\lib /NODEFAULTLIB:MSVCRT %libs% /OUT:%bindir%\Scum.exe 
popd
