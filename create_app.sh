#!bin/sh

echo Create a NAP app.
echo Usage: create_app.sh [name] [optional: directory]

# Check if app name is specified
if [ "$#" -lt "1" ]; then
  echo "Specify a name."
  exit 0
fi

app_name=$1
if [ $# = "1" ]; then
  app_directory="apps"
else
  app_directory=$2
fi

# Copy template to app dir
cp -r tools/buildsystem/template_app "$app_directory/$app_name"

# Replace template with app_name in module/src/naptemplate.cpp
sed "s/template/$app_name/g" "$app_directory/$app_name/module/src/naptemplate.cpp" > temp
mv temp "$app_directory/$app_name/module/src/nap$app_name.cpp"
rm "$app_directory/$app_name/module/src/naptemplate.cpp"

# Replace title and app module name in app.json
sed "s/template/$app_name/g" "$app_directory/$app_name/app.json" > temp
mv temp "$app_directory/$app_name/app.json"

echo "Your new app $app_name is now in the directory $app_directory and ready to be built."
