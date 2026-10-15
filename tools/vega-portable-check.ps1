param(
	[string]$OutputDir = (Join-Path $env:TEMP ('vega-portable-check-' + [Guid]::NewGuid().ToString('N')))
)

$ErrorActionPreference = 'Stop'
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$destination = [IO.Path]::GetFullPath($OutputDir)
if ($destination.StartsWith($repo + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase) -or $destination -eq $repo) {
	throw 'The portability check must run outside the repository.'
}
if (Test-Path -LiteralPath $destination) { throw 'Choose a new output folder; existing folders are never replaced.' }
$source = Join-Path $repo 'build-release/LightYearsGame'
New-Item -ItemType Directory -Path $destination | Out-Null
Copy-Item -LiteralPath (Join-Path $source 'LightYearsGame.exe') -Destination $destination
Get-ChildItem -LiteralPath $source -Filter '*.dll' -File | Copy-Item -Destination $destination
foreach ($folder in @('assets', 'visual')) {
	Copy-Item -LiteralPath (Join-Path $source $folder) -Destination $destination -Recurse
}
$bytes = (Get-ChildItem -LiteralPath $destination -File -Recurse | Measure-Object Length -Sum).Sum
$game = $null
try {
	$game = Start-Process -FilePath (Join-Path $destination 'LightYearsGame.exe') `
		-ArgumentList '--visual-capture check.png --capture-frames 120' `
		-WorkingDirectory $destination -WindowStyle Hidden -PassThru
	if (!$game.WaitForExit(60000)) { throw 'Portable game timed out after 60 seconds.' }
	if ($game.ExitCode -ne 0) { throw "Portable game exited with code $($game.ExitCode)." }
	$capture = Get-Item -LiteralPath (Join-Path $destination 'check.png')
	if (!$capture.Length) { throw 'Capture is empty.' }
	$log = Join-Path $destination 'LightYears.log'
	$expectedRoot = 'Packaged game and script asset root: ' + $destination.Replace('\', '/') + '/assets/'
	if (!(Select-String -LiteralPath $log -SimpleMatch $expectedRoot)) {
		throw 'The log does not confirm that game and script assets use the portable folder.'
	}
	$paths = Select-String -LiteralPath $log -Pattern 'D:[/\\]Vega',([regex]::Escape($repo.Replace('\', '/'))),([regex]::Escape($repo))
	if ($paths) { $paths | Out-String | Write-Output; throw 'Source-tree paths found in the portable game log.' }
	"Portable check passed. Folder: $destination; runtime bytes: $bytes; capture bytes: $($capture.Length); source-tree paths: 0"
}
finally {
	if ($game) {
		if (!$game.HasExited) { $game.Kill(); $game.WaitForExit() }
		$game.Dispose()
	}
}
