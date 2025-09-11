# PowerShell script to download and install MttVDD driver

# Create temporary directory
$tempDir = Join-Path $env:TEMP "VirtualDisplayDriverInstall"
New-Item -ItemType Directory -Path $tempDir -Force | Out-Null

# Create local directory for DevCon
$devconDir = Join-Path $tempDir "DevCon"
New-Item -ItemType Directory -Path $devconDir -Force | Out-Null

# Download DevCon Installer
$devconUrl = "https://github.com/Drawbackz/DevCon-Installer/releases/download/1.4-rc/Devcon.Installer.exe"
$devconPath = Join-Path $tempDir "Devcon.Installer.exe"
Write-Host "Downloading DevCon Installer..." -ForegroundColor Cyan
Invoke-WebRequest -Uri $devconUrl -OutFile $devconPath

# Run DevCon Installer with the specific hash parameter and local directory
Write-Host "Installing DevCon..." -ForegroundColor Cyan
Start-Process -FilePath $devconPath -ArgumentList "install -hash B85A2B81E1518EABCC4AFDEE145304CAD9708778C0EBFC177A2DB2847D7FB355 -update -dir `"$devconDir`"" -Wait -NoNewWindow

# Define path to devcon executable
$devconExe = Join-Path $devconDir "devcon.exe"

# Download driver package
$driverUrl = "https://github.com/VirtualDrivers/Virtual-Display-Driver/releases/download/24.12.24/Signed-Driver-v24.12.24-x64.zip"
$driverZipPath = Join-Path $tempDir "Signed-Driver-v24.12.24-x64.zip"
$driverExtractPath = Join-Path $tempDir "DriverFiles"

Write-Host "Downloading virtual display driver..." -ForegroundColor Cyan
Invoke-WebRequest -Uri $driverUrl -OutFile $driverZipPath

# Extract driver package
Write-Host "Extracting driver files..." -ForegroundColor Cyan
Expand-Archive -Path $driverZipPath -DestinationPath $driverExtractPath -Force

# Install the driver using the local devcon
Write-Host "Installing virtual display driver..." -ForegroundColor Cyan
Push-Location $driverExtractPath
$devconExe install .\MttVDD.inf "Root\MttVDD"
Pop-Location

Write-Host "Driver installation completed." -ForegroundColor Green