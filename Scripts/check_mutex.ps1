$mutexName = 'Global\LiveCoding_C++Program Files+Epic Games+UE_5.4+Engine+Binaries+Win64+UnrealEditor.exe'
$mutex = $null
$exists = [System.Threading.Mutex]::TryOpenExisting($mutexName, [ref]$mutex)
if ($exists) {
    Write-Host "Mutex EXISTS - closing it"
    $mutex.Close()
    $mutex.Dispose()
    Write-Host "Mutex closed"
} else {
    Write-Host "Mutex does NOT exist"
}
