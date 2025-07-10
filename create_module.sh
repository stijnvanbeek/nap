#!bin/sh

echo Create a NAP module.
echo Usage: create_module.sh [name] [optional: directory]

# Check if module name is specified
if [ "$#" -lt "1" ]; then
  echo "Specify a name."
  exit 0
fi

module_name=$1
if [ $# = "1" ]; then
  module_directory="modules"
else
  module_directory=$2
fi

# Copy template to module dir
cp -r tools/buildsystem/template_app/module "$module_directory/$module_name"

# Replace naptemplate with module_name in module/src/naptemplate.cpp
sed "s/naptemplate/$module_name/g" "$module_directory/$module_name/src/naptemplate.cpp" > temp
mv temp "$module_directory/$module_name/src/$module_name.cpp"
rm "$module_directory/$module_name/src/naptemplate.cpp"

echo "Your new module $module_name is now in the directory $module_directory and ready to be built."