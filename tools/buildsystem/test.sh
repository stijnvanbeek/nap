echo Test an executable.
echo Usage: sh test.sh [executable path] [arguments]

# Check if executable is specified
if [ "$#" -lt "1" ]; then
  echo "Specify an executable."
  exit 1
fi
executable=$1

# Start application
echo "Starting ${executable}..."
./${executable} $2 $3 $4 &
if ! [ $? -eq 0 ]; then
  exit $?
fi

# Determine process id
pid=$!
echo "pid: ${pid}"

# Check if app is running
kill -0 ${pid}
if ! [ $? -eq 0 ]; then
  exit $?
fi

echo "Waiting for ${executable} to run"
sleep 10

# Try to close app
echo "Trying to close ${executable}"
kill ${pid}
sleep 3

# Check if app is closed
echo "Checking if app is closed."
kill -0 ${pid}
if ! [ $? -eq 1 ]; then
  echo "Failed to close ${executable}"
  exit 2
fi
echo "${executable} successfully closed."


