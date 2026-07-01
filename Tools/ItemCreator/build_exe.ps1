# Rebuilds ItemCreator.exe from item_creator.py.
# Run after editing the tool:  powershell -ExecutionPolicy Bypass -File build_exe.ps1
$ErrorActionPreference = "Stop"
Set-Location $PSScriptRoot
python -m PyInstaller --onefile --windowed --name ItemCreator `
    --distpath dist --workpath build --specpath . item_creator.py
Write-Host "Built: $PSScriptRoot\dist\ItemCreator.exe"
