#!bin/sh

echo Package NAP app.
echo Usage: sh package_app.sh [target name]
echo Options:
echo "-b Build directory (If not specified \"build\" is used as default and will be removed when finished)"
echo "-z Zip the output"
echo "-d Delete the output (don't remove the optional zip)"
echo "-e Include the Napkin editor"
echo "-s MacOS code signature"
echo "-n MacOS notarization profile"
echo "-t Perform testing"

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

# Check if target is specified
if [ "$#" -lt "1" ]; then
  echo "Specify a target."
  exit 1
fi
target=$1

shift 1

# default
temp_build_directory=true
build_directory="build"
codesign=false
code_signature=""
notarize=false
notary_profile=""
zip_output=false
include_napkin=false
perform_testing=false
delete_output=false

while getopts 'b:s:n:zetd' OPTION; do
  case "$OPTION" in
    b)
      build_directory="$OPTARG"
      temp_build_directory=false
      echo "The build directory: $OPTARG"
      ;;
    s)
      export MACOS_CODE_SIGNATURE="$OPTARG"
      code_signature="$OPTARG"
      codesign=true
      echo "Using MacOS code signature: $OPTARG"
      ;;
    n)
      notary_profile="$OPTARG"
      notarize=true
      echo "Using MacOS notarization profile: $OPTARG"
      ;;
    z)
      zip_output=true
      echo "Output will be zipped."
      ;;
    e)
      include_napkin=true
      echo "Napkin editor will be included"
      ;;
    t)
      perform_testing=true
      echo "Testing enabled"
      ;;
    d)
      delete_output=true
      echo "Output will be deleted"
      ;;
    ?)
      exit 1
      ;;
  esac
done
echo

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

# Build napkin
if ! [ $target = "napkin" ]; then
  if [ $include_napkin = true ]; then
    echo Including napkin
    if [ "$(uname)" = "Darwin" ]; then
      echo Warning: MacOS app bundle structure might conflict with including napkin in the package.
    fi
    cmake --build $build_directory --target napkin --config Debug --parallel 8
    if ! [ $? -eq 0 ]; then
      exit $?
    fi
  fi
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
    app_directory=$app_title
  else
    app_title=Napkin
    app_directory=$app_title
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
echo "Cleaning previous install output... install/$app_directory"
rm -rf "install/$app_directory"

# Rename output directory to app title
if [ "$(uname)" = "Darwin" ]; then
  mv "install/MyApp.app" "install/$app_directory"
else
  mv "install/MyApp" "install/$app_directory"
fi

# Perform testing
if [ $perform_testing = true ]; then
  echo "Testing the app"
  if [ "$(uname)" = "Darwin" ]; then
    exe_dir=install/$app_directory/contents/macos
    app_data_dir=install/$app_directory/contents/resources
  else
    exe_dir=install/$app_directory
    app_data_dir=install/$app_directory
  fi
  sh tools/buildsystem/test.sh ${exe_dir}/${target}
  if ! [ $? -eq 0 ]; then
    echo "Test failed"
    exit 2
  fi

  if [ $include_napkin = true ]; then
    echo "Testing napkin"
    ./${exe_dir}/napkin --no-project-reopen --exit-after-load -p ${app_data_dir}/app.json
    if ! [ $? -eq 0 ]; then
      echo "Napkin test failed"
      exit 2
    fi
  fi
fi

if [ "$(uname)" = "Darwin" ]; then
  # Codesign MacOS app bundle
  if [ $codesign = true ]; then
    echo Codesigning MacOS bundle...
    codesign -s "$code_signature" -f "install/$app_directory" --options runtime
    if ! [ $? -eq 0 ]; then
      exit $?
    fi
  fi

  # Perform notarization
  if [ $notarize = true ]; then
    echo Performing MacOS notarization
    cd install

    # Zip app bundle to upload for notarization
    notary_zip="${app_title}.zip"
    zip -q -r "${notary_zip}" "${app_directory}"

    # Notarize
    xcrun notarytool submit "${notary_zip}" --keychain-profile "${notary_profile}" --wait
    if ! [ $? -eq 0 ]; then
      exit $?
    fi

    # Remove original app bundle and unzip notarized bundle
    rm -rf "${app_directory}"
    unzip -q "${notary_zip}"
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
if [ $zip_output = true ]; then
  echo "Zipping the output.."
  if [ "$(uname)" = "Darwin" ]; then
    app_zip="$app_title $app_version MacOS.zip"
    cd install
    zip -q -r "${app_zip}" "${app_directory}"
    cd ..
  elif [ "$(uname)" = "Linux" ]; then
    app_zip="$app_title $app_version Linux.zip"
    cd install
    zip -q -r "${app_zip}" "${app_directory}"
    cd ..
  else
    app_zip="$app_title $app_version Win.zip"
    cd install
    ../thirdparty/zip/msvc/zip -q -r "${app_zip}" "${app_directory}"
    cd ..
  fi
fi

# Remove output app folder
if [ $delete_output = true ]; then
  echo "Removing output app directory."
  rm -rf "install/${app_directory}"
fi

# Remove the build directory if it wasn't specified
if [ $temp_build_directory = true ]; then
  echo Removing build directory...
  rm -rf ${build_directory}
fi
