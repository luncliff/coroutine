<#
    Author
        github.com/luncliff (luncliff@gmail.com)

    Note
        The script is for Azure Pipelines environment

    See Also
        https://github.com/luncliff/coroutine/issues/15
        https://docs.microsoft.com/en-us/visualstudio/test/using-code-coverage-to-determine-how-much-code-is-being-tested?view=vs-2019

#>
$workspace = Get-Location

# Codecoverage.exe (Azure Pipelines)
if ("$workspace" -eq "D:\a\1\s") {
    # expect VS2017 image in Azure Pipelines
    $env:Path += ";C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\Team Tools\Dynamic Code Coverage Tools"
    Write-Output "using Azure Pipelines"
}

# Acquire coverage file and generate temporary coverage file
$coverage_file = Get-ChildItem -Recurse *.coverage;
if ($null -eq $coverage_file) {
    Write-Output "coverage file not found"
    return
}
Write-Output $coverage_file

$temp_coverage_xml_filepath = "./TestResults/coverage-report.xml"
CodeCoverage.exe analyze /verbose /output:$temp_coverage_xml_filepath $coverage_file

Tree ./TestResults /F

# Filter lines with invalid line number 
#   and Create a new coverage xml
$final_coverage_xml_filepath = "./TestResults/luncliff-coroutine.coveragexml"
$xml_lines = Get-Content $temp_coverage_xml_filepath
foreach ($text in $xml_lines) {
    if ($text -match 15732480) {
        Write-Output "removed $text"
        continue;
    }
    else {
        Add-Content $final_coverage_xml_filepath $text;
    }
}

Tree ./TestResults /F

# Display information of a new coverage xml
Get-ChildItem $final_coverage_xml_filepath
Get-Content   $final_coverage_xml_filepath
