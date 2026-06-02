#!/system/bin/sh
SKIPUNZIP=1

if [ "$BOOTMODE" ]; then
    ui_print "- Installing from ${KSU:+KernelSU}${APATCH:+APatch}${MAGISK:+Magisk} app"
else
    abort "Recovery install not supported"
fi

[ "$API" -lt 29 ] && abort "Minimal supported SDK is 29 (Android 10)"

case "$ARCH" in
    arm64) ARCH_DIR="arm64-v8a" ;;
    arm)   ARCH_DIR="armeabi-v7a" ;;
    *)     abort "Unsupported arch: $ARCH" ;;
esac

VERSION=$(grep_prop version "$TMPDIR/module.prop")
ui_print "- ForgeMint $VERSION on $ARCH"

mkdir -p "$MODPATH/lib" "$MODPATH/bin"
ui_print "- Extracting module files"
unzip -o "$ZIPFILE" "module.prop" -d "$MODPATH" >&2
unzip -o "$ZIPFILE" "lib/$ARCH_DIR/libforgemint.so" -d "$MODPATH" >&2
mv "$MODPATH/lib/$ARCH_DIR/libforgemint.so" "$MODPATH/lib/" && rmdir "$MODPATH/lib/$ARCH_DIR"
unzip -o "$ZIPFILE" "bin/$ARCH_DIR/injector" -d "$MODPATH" >&2
mv "$MODPATH/bin/$ARCH_DIR/injector" "$MODPATH/bin/" && rmdir "$MODPATH/bin/$ARCH_DIR"
unzip -o "$ZIPFILE" "keys/*" -d "$MODPATH" >&2
unzip -o "$ZIPFILE" "convert.sh" -d "$MODPATH" >&2
unzip -o "$ZIPFILE" "sepolicy.rule" -d "$MODPATH" >&2
unzip -o "$ZIPFILE" "service.sh" -d "$MODPATH" >&2

ui_print "- Setting permissions"
set_perm_recursive "$MODPATH/lib" 0 0 0644 0644
set_perm_recursive "$MODPATH/bin" 0 0 0755 0755
set_perm_recursive "$MODPATH/keys" 0 0 0644 0644
set_perm "$MODPATH/convert.sh"   0 0 0755
set_perm "$MODPATH/service.sh"   0 0 0755
set_perm "$MODPATH/bin/injector" 0 0 0755

ui_print "- Done"
