# Run in sudo mode on linux before starting to work with the VST3 sdk

if [ "$(uname)" = "Linux" ]; then
  apt install libx11-xcb-dev
  apt install libxcb-util-dev
  apt install libxcb-cursor-dev
  apt install libxcb-xkb-dev
  apt install libxkbcommon-dev
  apt install libxkbcommon-x11-dev
  apt install libfontconfig1-dev
  apt install libcairo2-dev
  apt install libgtkmm-3.0-dev
  apt install libsqlite3-dev
  apt install libxcb-keysyms1-dev
fi
