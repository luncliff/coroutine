# Path to Codecoverage.exe
$env:Path += ";C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\Team Tools\Dynamic Code Coverage Tools"

# Acquire coverage file and generate temporary coverage file
$coverage_file = Get-ChildItem -Recurse *.coverage;
Write-Host $coverage_file
$temp_coverage_xml_filepath  = "./TestResults/coverage-report.xml"
CodeCoverage.exe analyze /output:$temp_coverage_xml_filepath $coverage_file

Tree ./TestResults /F

# Filter lines with invalid line number 
#   and Create a new coverage xml
$final_coverage_xml_filepath = "./TestResults/luncliff-coroutine-visual-studio.coveragexml"
$xml_lines = Get-Content $temp_coverage_xml_filepath
foreach($text in $xml_lines){
    if($text -match 15732480){
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
