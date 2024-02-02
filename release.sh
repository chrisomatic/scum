source ./build.sh

cd ..
rm -rf release
mkdir -p release/src/core/
mkdir -p release/bin

cp -r src/core/fonts ./release/src/core/
cp -r src/core/shaders ./release/src/core/

cp -r src/img ./release/src/
cp -r src/effects ./release/src/
cp -r src/themes ./release/src/
cp -r src/rooms ./release/src/

cp bin/scum ./release/bin/
echo "#!/bin/sh" > ./release/run.sh
echo "./bin/scum" >> ./release/run.sh
chmod +x ./release/run.sh

zip -r scum_linux.zip release/* 

echo "Done. Run 'cd release && ./run.sh'"
