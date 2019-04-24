#!/bin/bash
rm -rf out
rm -rf tools
mkdir -p out
mkdir -p tools

echo "building for windows"
CGO_ENABLED=1 CC=x86_64-w64-mingw32-gcc GOOS=windows GOARCH=amd64 go build -tags static -ldflags "-s -w"
mv hid_mitm.exe tools/input_pc_win.exe
echo "building for linux"
CGO_ENABLED=1 CC=gcc GOOS=linux GOARCH=amd64 go build -tags static -ldflags "-s -w"
mv hid_mitm tools/input_pc_linux
echo "building for switch"
pushd ..
make -j
popd
mkdir -p out/atmosphere/titles/0100000000000faf/flags
mkdir -p out/config/hid_mitm
cp config.ini out/config/hid_mitm
touch out/atmosphere/titles/0100000000000faf/flags/boot2.flag
cp hid_mitm.nsp out/atmosphere/titles/0100000000000faf/exefs.nsp
cp start.bat tools/
cp hid-mitm.ipa tools
cp hid-mitm.apk tools


rm -r hid-mitm.zip 2>/dev/null
pushd out
zip -r ../hid-mitm.zip *
popd

rm -r companion_apps.zip 2>/dev/null
pushd tools
zip -r ../companion_apps.zip *
popd