#!/bin/sh
# Fix install names and inter-dylib dependencies of the ffmpeg dylib set to @rpath/...
set -eu

LIBDIR="/Users/stijn/Documents/GitHub/nap/system_modules/napvideo/thirdparty/ffmpeg/macos/universal/lib"

cd "$LIBDIR"

# Real dylib files only (skip the versionless symlinks)
LIBS=$(find . -maxdepth 1 -type f -name "*.dylib" -exec basename {} \; | sort)

# 1. Set each dylib's own install name to @rpath/<filename>
for lib in $LIBS; do
  install_name_tool -id "@rpath/$lib" "$lib"
done

# 2. Rewrite dependencies that point at one of our own dylibs
for lib in $LIBS; do
  DEPS=$(otool -L "$lib" | tail -n +2 | awk '{print $1}')
  for dep in $DEPS; do
    case "$dep" in
      @rpath/*|@loader_path/*|@executable_path/*) continue ;;
      /System/*|/usr/lib/*)                      continue ;;  # system libs stay as-is
    esac

    # Only rewrite if the basename matches a real dylib sitting next to us
    if [ -f "$dep" ] || [ -f "$LIBDIR/$(basename "$dep")" ]; then
      install_name_tool -change "$dep" "@rpath/$(basename "$dep")" "$lib"
    fi
  done
done

# 3. Re-sign each modified dylib (required on Apple Silicon)
for lib in $LIBS; do
  codesign --force --sign - "$lib"
done

echo "Done. Current state:"
for lib in $LIBS; do
  echo "--- $lib"
  otool -D "$lib"
  otool -L "$lib" | head -n 12
done
