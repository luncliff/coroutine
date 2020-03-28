<#
    Author
        github.com/luncliff (luncliff@gmail.com)

    Note
        Download SonarCloud build wrapper

    See Also
        scripts/sonar-wrapper-build.ps1
#>

# Allow execution of downloaded script
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
Invoke-WebRequest -Uri http://sonarcloud.io/static/cpp/build-wrapper-win-x86.zip -OutFile build-wrapper-win.zip

Expand-Archive -Path .\build-wrapper-win.zip -DestinationPath .
Copy-Item .\build-wrapper-win-x86\build-wrapper-win-x86-64.exe .
