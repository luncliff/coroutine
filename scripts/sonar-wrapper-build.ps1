<#
    Author
        github.com/luncliff (luncliff@gmail.com)

    Note
        The script is for Azure Pipelines environment
        Generate a Visual Studio solution file if not exists. 
        Execute SonarCloud build wrapper with it.

    See Also
        scripts/create-coerage-xml.ps1

#>
$workspace = Get-Location
$msbuild_exe = "MSBuild.exe"
# MSBuild.exe (Azure Pipelines)
if ("$workspace" -eq "D:\a\1\s") {
    # expect VS2017 image in Azure Pipelines
    $env:Path += ";C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\MSBuild\15.0\Bin"
    Write-Output "using Azure Pipelines"
}
# if not found, we can't advance ...
$command = Get-Command "$msbuild_exe"
Write-Output $command

$msbuild_exe = $command.Source
Write-Output "using MSBuild: $(MSBuild -version)"

$solution_file = "coroutine.sln"
$target_platform = "x64"
$target_configuration = "Debug"

if (-Not (Test-Path "$solution_file")) {
    cmake . `
        -A "$target_platform" -DCMAKE_BUILD_TYPE="$target_configuration"  `
        -DBUILD_SHARED_LIBS=false -DBUILD_TESTING=true
}
else {
    Write-Output "using solution file: $solution_file"
}

# this is intended path for SonarCloud job in Azure Pipelines
$output_dir = Join-Path ".." "bw-output"
Write-Output "$workspace --> $output_dir"

./build-wrapper-win-x86-64.exe --out-dir "$output_dir" `
    "$msbuild_exe" "$solution_file" /t:rebuild /p:platform="$target_platform" /p:configuration="$target_configuration"
