# Runs one Sol 6.1 (Codex) task in a new visible PowerShell window so the user can watch it live.
# Claude writes the task file, starts this script, waits for the done file, then reviews the diff, builds and tests.
# Task files and results live in <repo>\.sol\ (ignored by git). The final answer goes to <name>.out.md and the
# exit code to <name>.done when the run ends.
#
#   tools\sol\Invoke-Sol.ps1 -Task .sol\task-003-foo.txt -Sandbox workspace-write -Effort medium
param(
    [Parameter(Mandatory = $true)][string]$Task,
    [ValidateSet('read-only', 'workspace-write')][string]$Sandbox = 'read-only',
    # Pick by difficulty; never above high (user decision 2026-10-15).
    [ValidateSet('low', 'medium', 'high')][string]$Effort = 'medium',
    [string]$WorkDir = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path,
    [string]$Title = 'Sol 6.1 - Vega'
)

$ErrorActionPreference = 'Stop'

$codex = Get-ChildItem "$env:LOCALAPPDATA\OpenAI\Codex\bin" -Recurse -Filter codex.exe |
    Sort-Object LastWriteTime -Descending |
    Select-Object -First 1 -ExpandProperty FullName
if (-not $codex) { throw 'codex.exe not found under %LOCALAPPDATA%\OpenAI\Codex\bin' }

$taskPath = (Resolve-Path $Task).Path
$stem = Join-Path (Split-Path $taskPath) ([System.IO.Path]::GetFileNameWithoutExtension($taskPath))
$outFile = "$stem.out.md"
$doneFile = "$stem.done"
$runner = "$stem.runner.ps1"
if (Test-Path $doneFile) { Remove-Item $doneFile -Force }

$body = @"
`$utf8 = New-Object System.Text.UTF8Encoding `$false
`$OutputEncoding = `$utf8
[Console]::OutputEncoding = `$utf8
`$Host.UI.RawUI.WindowTitle = '$Title'
Write-Host '=== Claude -> Sol 6.1 gorevi ===' -ForegroundColor Cyan
Write-Host ('Klasor: $WorkDir   Sandbox: $Sandbox   Effort: $Effort') -ForegroundColor DarkCyan
Get-Content -Raw -Encoding UTF8 '$taskPath' | Write-Host
Write-Host '================================' -ForegroundColor Cyan
Get-Content -Raw -Encoding UTF8 '$taskPath' | & '$codex' exec -m gpt-6.1-sol -c 'model_reasoning_effort="$Effort"' --sandbox $Sandbox --skip-git-repo-check -C '$WorkDir' --color always -o '$outFile' -
`$code = `$LASTEXITCODE
Set-Content -Encoding ASCII -Path '$doneFile' -Value `$code
Write-Host ''
Write-Host "=== Bitti (exit `$code). Sonucu Claude okuyacak; pencereyi kapatabilirsin. ===" -ForegroundColor Green
"@
Set-Content -Encoding UTF8 -Path $runner -Value $body

Start-Process powershell.exe -WorkingDirectory $WorkDir -ArgumentList @('-NoExit', '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', "`"$runner`"")
Write-Output "Started Sol window. Done file: $doneFile"
