#!bin/sh

echo Package NAP app.
echo Usage: sh package_app.sh [target] [optional: build directory] [optional: MacOS code signature]

# Check if target is specified
if [ "$#" -lt "1" ]; then
  echo "Specify a target."
  exit 1
fi

# Make sure cmake is installed
if ! [ -x "$(command -v cmake)" ]; then
  echo Cmake is not installed. Install it for your system.
  exit 1
fi

# Make sure jq is installed on unix
if [ "$(uname)" = "Darwin" ]; then
  if ! [ -x "$(command -v jq)" ]; then
    echo Jq json parser not found. To install from homebrew run:
    echo brew install jq
    exit 1
  fi
elif [ "$(uname)" = "Linux" ]; then
  if ! [ -x "$(command -v jq)" ]; then
    echo Jq json parser not found. To install from package manager run:
    echo sudo apt install jq
    exit 1
  fi
#else
  # Windows
  # curl -L -o jq.exe https://github.com/stedolan/jq/releases/latest/download/jq-win64.exe
fi

target=$1
if [ $# = "1" ]; then
  build_directory="build"
else
  build_directory=$2
fi

# If there is a code signature provided pass it on as environment variable
if [ "$(uname)" = "Darwin" ]; then
  if [ "$#" -gt "2" ]; then
    echo Using signature: $3
    export MACOS_CODE_SIGNATURE="$3"
  fi
fi

# Remove bin directory from previous builds
# This is important otherwise artifacts from previous builds could be included in the app installation
echo Cleaning previous build output...
rm -rf $build_directory/bin

# Generate the build directory
cmake -S . -B $build_directory -DCMAKE_BUILD_TYPE=RELEASE
if ! [ $? -eq 0 ]; then
  exit $?
fi

# Build the specified target
cmake --build $build_directory --target $target --config Release --parallel 8
if ! [ $? -eq 0 ]; then
  exit $?
fi

# Run cmake install process
cmake --install $build_directory --prefix install
if ! [ $? -eq 0 ]; then
  exit $?
fi

# Read app Title and Version from project json
if [ "$target" = "napkin" ]; then
  if [ "$(uname)" = "Darwin" ]; then
    # Add app bundle file extension on MacOS
    app_title=Napkin.app
  else
    app_title=Napkin
  fi
else
  if [ "$(uname)" = "Darwin" ]; then
    # Add app bundle file extension on MacOS
    app_title=`jq -r '.Title' $build_directory/bin/$target.json`
    app_version=`jq -r '.Version' $build_directory/bin/$target.json`
    app_directory="$app_title.app"
  elif [ "$(uname)" = "Linux" ]; then
    app_title=`jq -r '.Title' $build_directory/bin/$target.json`
    app_version=`jq -r '.Version' $build_directory/bin/$target.json`
    app_directory=$app_title
  else
    # Use bundled jq.exe
    app_title=`./thirdparty/jq/msvc/x86_64/jq.exe -r '.Title' $build_directory/bin/$target.json`
    app_version=`./thirdparty/jq/msvc/x86_64/jq.exe -r '.Version' $build_directory/bin/$target.json`
    app_directory=$app_title
  fi
  if ! [ $? -eq 0 ]; then
    exit $?
  fi
fi
echo App title is: $app_title
echo App version is: $app_version

# Cleaning previous install, if any
echo Cleaning previous install output...
rm -rf "install/$app_directory"

# Rename output directory to app title
if [ "$(uname)" = "Darwin" ]; then
  mv "install/MyApp.app" "install/$app_directory"
else
  mv "install/MyApp" "install/$app_directory"
fi

if [ "$(uname)" = "Darwin" ]; then
  # Codesign MacOS app bundle
  if [ "$#" -gt "2" ]; then
    echo Codesigning MacOS bundle...
    codesign -s "$3" -f "install/$app_directory" --options runtime
    if ! [ $? -eq 0 ]; then
      exit $?
    fi
  fi

  # Perform notarization
  if [ "$#" -gt "3" ]; then
    echo Performing MacOS notarization
    notary_profile=$4
    cd install

    # Zip app bundle to upload for notarization
    notary_zip="${app_title}.zip"
    echo zipping "${notary_zip}" "${app_directory}"
    zip -r "${notary_zip}" "${app_directory}"

    # Notarize
    xcrun notarytool submit "${notary_zip}" --keychain-profile "${notary_profile}" --wait
    if ! [ $? -eq 0 ]; then
      exit $?
    fi

    # Remove original app bundle and unzip notarized bundle
    rm -rf "${app_directory}"
    unzip "${notary_zip}"
    rm "${notary_zip}"

    # Run stapler
    xcrun stapler staple "${app_directory}"
    if ! [ $? -eq 0 ]; then
      exit $?
    fi

    cd ..
  fi
fi

# Zip the output directory
if [ "$(uname)" = "Darwin" ]; then
  app_zip="$app_title $app_version MacOS.zip"
  cd install
  zip -r "${app_zip}" "${app_directory}"
  cd ..
elif [ "$(uname)" = "Linux" ]; then
  app_zip="$app_title $app_version Linux.zip"
  cd install
  zip -r "${app_zip}" "${app_directory}"
  cd ..
else
  app_zip="$app_title $app_version Win.zip"
  cd install
  ../thirdparty/zip/msvc/zip -r "${app_zip}" "${app_directory}"
  cd ..
fi

# Remove the build directory if it wasn't specified
if [ $# = "1" ]; then
  echo Removing build directory...
  rm -rf build
fi
