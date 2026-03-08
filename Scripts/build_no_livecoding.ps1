# Kill the Live Coding mutex and immediately build
$mutexName = 'Global\LiveCoding_C++Program Files+Epic Games+UE_5.4+Engine+Binaries+Win64+UnrealEditor.exe'

# Try to close the mutex
$mutex = $null
$exists = [System.Threading.Mutex]::TryOpenExisting($mutexName, [ref]$mutex)
if ($exists) {
    Write-Host "Found Live Coding mutex - releasing handle"
    $mutex.ReleaseMutex()
    $mutex.Close()
    $mutex.Dispose()
    Write-Host "Handle released"
}

# Also try to create and immediately destroy it (take ownership then release)
try {
    $createdNew = $false
    $m = New-Object System.Threading.Mutex($true, $mutexName, [ref]$createdNew)
    if (-not $createdNew) {
        Write-Host "Mutex existed, took ownership"
    } else {
        Write-Host "Created new mutex (it was gone)"
    }
    $m.ReleaseMutex()
    $m.Close()
    $m.Dispose()
    Write-Host "Mutex fully released"
} catch {
    Write-Host "Mutex manipulation failed: $_"
}

# Small delay to ensure OS cleanup
Start-Sleep -Milliseconds 200

# Verify it's gone
$mutex2 = $null
$exists2 = [System.Threading.Mutex]::TryOpenExisting($mutexName, [ref]$mutex2)
if ($exists2) {
    Write-Host "WARNING: Mutex still exists after cleanup!"
    $mutex2.Close()
} else {
    Write-Host "Mutex confirmed gone"
}

# Now build
Write-Host "`n=== Starting Build ==="
& "C:\Program Files\Epic Games\UE_5.4\Engine\Build\BatchFiles\Build.bat" TheSignalEditor Win64 Development "C:\Users\Ommei\workspace\TheSignal\TheSignal.uproject" -WaitMutex
