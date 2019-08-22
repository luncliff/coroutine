# Download build-wrapper
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
Invoke-WebRequest -Uri http://sonarcloud.io/static/cpp/build-wrapper-win-x86.zip -OutFile build-wrapper-win.zip;

Expand-Archive -Path .\build-wrapper-win.zip -DestinationPath ./ ;
Copy-Item .\build-wrapper-win-x86\build-wrapper-win-x86-64.exe ./ ;
