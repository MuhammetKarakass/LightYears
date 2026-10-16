# Original geometric art; System.Drawing is used only to generate the checked-in PNG.
Add-Type -AssemblyName System.Drawing
$tile = New-Object Drawing.Bitmap 32,32
for ($y = 0; $y -lt 32; ++$y)
{
	for ($x = 0; $x -lt 32; ++$x)
	{
		$color = [Drawing.Color]::FromArgb(255,51,199,202)
		if ($x -lt 2 -or $y -lt 2 -or $x -ge 30 -or $y -ge 30)
		{
			$color = [Drawing.Color]::White
		}
		$tile.SetPixel($x,$y,$color)
	}
}
$tile.Save((Join-Path $PSScriptRoot '../Assets/textures/tile.png'), [Drawing.Imaging.ImageFormat]::Png)
$tile.Dispose()
