source ./build.sh



cd ..
rm -rf output
mkdir -p output/src/core/
mkdir -p output/bin

cp -r src/core/fonts ./output/src/core/
cp -r src/core/shaders ./output/src/core/

cp -r src/img ./output/src/
cp -r src/effects ./output/src/
cp -r src/themes ./output/src/
cp -r src/rooms ./output/src/

cp bin/scum ./output/bin/
echo "#!/bin/sh" > ./output/run.sh
echo "./bin/scum" >> ./output/run.sh
chmod +x ./output/run.sh

zip -r scum_linux.zip output/* 

echo "Done. Run 'cd output && ./run.sh'"
