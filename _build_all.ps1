$ErrorActionPreference = 'Continue'
$base = 'D:\HJZ'
$gcc = 'D:\bin\gcc.exe'
$results = @()

$projects = Get-ChildItem $base -Directory | Where-Object { $_.Name -match '^Project_(\d+)' -and $_.Name -notmatch '^Project_39' -and $_.Name -notmatch '^Project_31$' } | Sort-Object Name

foreach ($p in $projects) {
    $makefile = Join-Path $p.FullName 'Makefile'
    $mainc = Join-Path $p.FullName 'main.c'
    if (-not (Test-Path $mainc)) { continue }
    
    $exeName = $null
    $ldflags = ''
    if (Test-Path $makefile) {
        $mk = Get-Content $makefile -Raw
        if ($mk -match 'TARGET\s*=\s*(\S+)') { $exeName = $Matches[1] }
        if ($mk -match 'LDFLAGS\s*=\s*(.+?)(\r?\n|$)') { $ldflags = $Matches[1].Trim() }
    }
    if (-not $exeName) { $exeName = 'main.exe' }
    
    $exePath = Join-Path $p.FullName $exeName
    Remove-Item -ErrorAction SilentlyContinue $exePath, (Join-Path $p.FullName '*.o')
    
    Push-Location $p.FullName
    $warnOut = & $gcc -std=c99 -Wall -Wextra -O2 -o $exeName main.c $ldflags.Split() 2>&1
    $exitCode = $LASTEXITCODE
    Pop-Location
    
    $warnings = if ($warnOut) { ($warnOut | Where-Object { $_ -match 'warning:|error:' }) -join ' | ' } else { '' }
    $hasWarn = ($warnings -ne '') -or ($exitCode -ne 0)
    $exists = Test-Path $exePath
    
    $results += [PSCustomObject]@{
        Project = $p.Name
        Exe = $exeName
        BuildOk = $exists -and (-not $hasWarn)
        ExitCode = $exitCode
        Warnings = $warnings
    }
}

$results | Format-Table -AutoSize -Property Project, Exe, BuildOk, ExitCode
$fail = $results | Where-Object { -not $_.BuildOk }
Write-Host "`n=== Build Summary: $(($results | Where-Object BuildOk).Count)/$($results.Count) OK, $($fail.Count) FAILED ==="
if ($fail) {
    Write-Host "`n=== FAILURES ==="
    $fail | ForEach-Object { Write-Host "[$($_.Project)]: $($_.Warnings)" }
}
