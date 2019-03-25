#!/bin/bash
mkdir -p out

echo "building for windows"
CGO_ENABLED=1 CC=x86_64-w64-mingw32-gcc GOOS=windows GOARCH=amd64 go build -tags static -ldflags "-s -w"
mv hid_mitm.exe out/input_pc_win.exe
echo "building for linux"
CGO_ENABLED=1 CC=gcc GOOS=linux GOARCH=amd64 go build -tags static -ldflags "-s -w"
mv hid_mitm out/input_pc_linux
echo "building for switch"
pushd ..
make -j
popd
mkdir -p out/atmosphere/titles/0100000000000123/flags
mkdir -p out/modules/hid_mitm
cp config.ini out/modules/hid_mitm
touch out/atmosphere/titles/0100000000000123/flags/boot2.flag
cp hid_mitm.nsp out/atmosphere/titles/0100000000000123/exefs.nsp
cp start.bat out/

rm -r build.zip
pushd out
zip -r ../build.zip *
popd