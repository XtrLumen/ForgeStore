#!/bin/sh
# build.sh — build libforgemint.so with NDK, then package module ZIP
# Prerequisites: Android NDK installed, set ANDROID_NDK_HOME
# Usage: ./build.sh [arm64|arm]

set -e

ABI="${1:-arm64-v8a}"
echo "Building for $ABI"

# Locate NDK
NDK="${ANDROID_NDK_HOME:-$HOME/Android/Sdk/ndk}"
[ -d "$NDK" ] || NDK=$(ls -d /opt/android-ndk* 2>/dev/null | head -1)
[ -d "$NDK" ] || NDK=$(ls -d $HOME/android-ndk* 2>/dev/null | head -1)
[ -d "$NDK" ] || { echo "NDK not found. Set ANDROID_NDK_HOME"; exit 1; }

CMAKE_TOOLCHAIN="$NDK/build/cmake/android.toolchain.cmake"
[ -f "$CMAKE_TOOLCHAIN" ] || { echo "toolchain not found at $CMAKE_TOOLCHAIN"; exit 1; }

# Configure ABIs
case "$ABI" in
    arm64-v8a) ABI_DIR="arm64-v8a"; API_LEVEL=29 ;;
    armeabi-v7a) ABI_DIR="armeabi-v7a"; API_LEVEL=29 ;;
    *) echo "Unsupported ABI: $ABI"; exit 1 ;;
esac

BUILD_DIR="build_$ABI_DIR"
mkdir -p "$BUILD_DIR"

cmake -S src -B "$BUILD_DIR" \
    -DCMAKE_TOOLCHAIN_FILE="$CMAKE_TOOLCHAIN" \
    -DANDROID_ABI="$ABI_DIR" \
    -DANDROID_PLATFORM=android-$API_LEVEL \
    -DCMAKE_BUILD_TYPE=MinSizeRel

cmake --build "$BUILD_DIR" --target forgemint -j$(nproc)
cmake --build "$BUILD_DIR" --target injector -j$(nproc)

# Copy outputs
mkdir -p "lib/$ABI_DIR" "bin/$ABI_DIR"
cp "$BUILD_DIR/libforgemint.so" "lib/$ABI_DIR/"
cp "$BUILD_DIR/injector" "bin/$ABI_DIR/"

# Strip
STRIP="$NDK/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-strip"
"$STRIP" "lib/$ABI_DIR/libforgemint.so" "bin/$ABI_DIR/injector" 2>/dev/null || true
echo "Built: lib/$ABI_DIR/libforgemint.so ($(wc -c < "lib/$ABI_DIR/libforgemint.so") bytes)"
echo "Built: bin/$ABI_DIR/injector ($(wc -c < "bin/$ABI_DIR/injector") bytes)"

# Package module
echo "Packaging module..."
MODID=$(grep '^id=' module.prop | cut -d= -f2)
VER=$(grep '^version=' module.prop | cut -d= -f2)
OUTDIR="build/$MODID"
rm -rf "$OUTDIR"
mkdir -p "$OUTDIR"

cp -f module.prop customize.sh sepolicy.rule service.sh convert.sh "$OUTDIR/" 2>/dev/null || true
cp -rf lib bin keys "$OUTDIR/"

chmod 755 "$OUTDIR"/*.sh 2>/dev/null || true

mkdir -p "$OUTDIR/META-INF/com/google/android"
echo "#MAGISK" > "$OUTDIR/META-INF/com/google/android/updater-script"

cd "$OUTDIR"
zip -r9 "../${MODID}-${VER}.zip" . >/dev/null
cd ..
rm -rf "$MODID"
echo "Package: build/${MODID}-${VER}.zip"
