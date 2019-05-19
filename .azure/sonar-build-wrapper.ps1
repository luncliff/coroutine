# Download build-wrapper
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
Invoke-WebRequest -Uri http://sonarcloud.io/static/cpp/build-wrapper-win-x86.zip -OutFile build-wrapper-win.zip;

Expand-Archive -Path .\build-wrapper-win.zip -DestinationPath ./ ;
Copy-Item .\build-wrapper-win-x86\build-wrapper-win-x86-64.exe ./ ;

# # Set Path to MSBuild
# $env:Path += ";C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\MSBuild\15.0\Bin\amd64"

# # Run build-wrapper
# ./build-wrapper-win-x86-64.exe --out-dir ../bw-output MSBuild.exe /t:rebuild  /p:platform="x64" /p:configuration="Debug";
