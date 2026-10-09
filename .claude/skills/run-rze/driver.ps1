<#
Drives the RZE Editor (native Win32 + DX11 + Dear ImGui) with real mouse/keyboard input.
One action per invocation; state lives in the running Editor.exe process.

  driver.ps1 launch [-Scene DrawLineTest.scene] [-Config debug] [-SettleSeconds 5]
  driver.ps1 shot   [-Out <path.png>]          # client-area screenshot; default $env:TEMP\rze-shots\<timestamp>.png
  driver.ps1 click  -X <int> -Y <int>          # client-area pixel coords (same space as screenshots)
  driver.ps1 drag   -X <int> -Y <int> -X2 <int> -Y2 <int>
  driver.ps1 key    -Keys '<SendKeys string>'  # e.g. '^a45{ENTER}' to replace an ImGui float field
  driver.ps1 info                              # window title + client size
  driver.ps1 alive                             # running / responding?
  driver.ps1 quit                              # kills Editor.exe (unsaved scene edits are discarded)
#>
param(
	[Parameter(Mandatory, Position = 0)]
	[ValidateSet('launch', 'shot', 'click', 'drag', 'key', 'info', 'alive', 'quit')]
	[string] $Action,
	[string] $Scene = 'DrawLineTest.scene',
	[ValidateSet('debug', 'release')] [string] $Config = 'debug',
	[int] $SettleSeconds = 5,
	[int] $TimeoutSeconds = 90,
	[string] $Out,
	[int] $X, [int] $Y,
	[int] $X2, [int] $Y2,
	[string] $Keys
)

$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path
$buildDir = Join-Path $repoRoot "RZE\_build\$Config"

Add-Type -AssemblyName System.Drawing, System.Windows.Forms
if (-not ('RzeWin' -as [type])) {
	Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class RzeWin {
	[StructLayout(LayoutKind.Sequential)] public struct RECT { public int L, T, R, B; }
	[StructLayout(LayoutKind.Sequential)] public struct POINT { public int X, Y; }
	[DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr h, out RECT r);
	[DllImport("user32.dll")] public static extern bool ClientToScreen(IntPtr h, ref POINT p);
	[DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
	[DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int c);
	[DllImport("user32.dll")] public static extern bool IsIconic(IntPtr h);
	[DllImport("user32.dll")] public static extern bool SetCursorPos(int x, int y);
	[DllImport("user32.dll")] public static extern void mouse_event(uint f, int x, int y, uint d, UIntPtr e);
	[DllImport("user32.dll")] public static extern bool SetProcessDPIAware();
}
"@
}
# Physical pixels everywhere, so screenshot pixels == click coordinates
[RzeWin]::SetProcessDPIAware() | Out-Null

function Get-EditorProcess {
	Get-Process Editor -ErrorAction SilentlyContinue | Where-Object { $_.MainWindowHandle -ne 0 } | Select-Object -First 1
}

function Get-Window {
	$proc = Get-EditorProcess
	if (-not $proc) { throw 'Editor.exe is not running (or has no window yet). Run: driver.ps1 launch' }
	$h = $proc.MainWindowHandle
	$cr = New-Object RzeWin+RECT; [RzeWin]::GetClientRect($h, [ref]$cr) | Out-Null
	$origin = New-Object RzeWin+POINT; [RzeWin]::ClientToScreen($h, [ref]$origin) | Out-Null
	[pscustomobject]@{ Proc = $proc; Handle = $h; Width = $cr.R; Height = $cr.B; Origin = $origin }
}

function Focus($w) {
	# Only un-minimize. SW_RESTORE on a maximized window un-maximizes it and reflows the whole ImGui layout.
	if ([RzeWin]::IsIconic($w.Handle)) { [RzeWin]::ShowWindow($w.Handle, 9) | Out-Null }
	[RzeWin]::SetForegroundWindow($w.Handle) | Out-Null
	Start-Sleep -Milliseconds 150
}

function MoveTo($w, $cx, $cy) {
	[RzeWin]::SetCursorPos($w.Origin.X + $cx, $w.Origin.Y + $cy) | Out-Null
	Start-Sleep -Milliseconds 60
}

switch ($Action) {
	'launch' {
		if (Get-Process Editor -ErrorAction SilentlyContinue) { throw 'Editor.exe already running. Run: driver.ps1 quit' }
		$exe = Join-Path $buildDir 'Editor.exe'
		if (-not (Test-Path $exe)) { throw "Not built: $exe" }
		# Working directory must be the build dir: Assets/ and ProjectData/ are resolved relative to it
		$p = Start-Process -FilePath $exe -ArgumentList "-scene $Scene" -WorkingDirectory $buildDir -PassThru
		$deadline = (Get-Date).AddSeconds($TimeoutSeconds)
		while ((Get-Date) -lt $deadline) {
			Start-Sleep -Milliseconds 500
			$p.Refresh()
			if ($p.HasExited) { throw "Editor.exe exited during startup (code $($p.ExitCode))" }
			if ($p.MainWindowTitle -like "*$Scene") { break }
		}
		if ($p.MainWindowTitle -notlike "*$Scene") { throw "Timed out waiting for window title to show $Scene (title: '$($p.MainWindowTitle)')" }
		# Title is set when the load starts; meshes/textures stream in over the following frames
		Start-Sleep -Seconds $SettleSeconds
		$w = Get-Window
		"launched pid=$($p.Id) title='$($p.MainWindowTitle)' client=$($w.Width)x$($w.Height)"
	}
	'info' {
		$w = Get-Window
		"title='$($w.Proc.MainWindowTitle)' client=$($w.Width)x$($w.Height) origin=$($w.Origin.X),$($w.Origin.Y)"
	}
	'alive' {
		$proc = Get-Process Editor -ErrorAction SilentlyContinue
		if (-not $proc) { 'not running' } else { "running pid=$($proc.Id) responding=$($proc.Responding)" }
	}
	'shot' {
		$w = Get-Window
		Focus $w
		if (-not $Out) {
			$dir = Join-Path $env:TEMP 'rze-shots'
			New-Item -ItemType Directory -Force $dir | Out-Null
			$Out = Join-Path $dir ("{0:yyyyMMdd-HHmmss-fff}.png" -f (Get-Date))
		}
		$bmp = New-Object System.Drawing.Bitmap $w.Width, $w.Height
		$g = [System.Drawing.Graphics]::FromImage($bmp)
		$g.CopyFromScreen($w.Origin.X, $w.Origin.Y, 0, 0, $bmp.Size)
		$bmp.Save($Out, [System.Drawing.Imaging.ImageFormat]::Png)
		$g.Dispose(); $bmp.Dispose()
		"saved $Out ($($w.Width)x$($w.Height))"
	}
	'click' {
		$w = Get-Window
		Focus $w; MoveTo $w $X $Y
		[RzeWin]::mouse_event(0x2, 0, 0, 0, [UIntPtr]::Zero); Start-Sleep -Milliseconds 60
		[RzeWin]::mouse_event(0x4, 0, 0, 0, [UIntPtr]::Zero); Start-Sleep -Milliseconds 100
		"clicked $X,$Y"
	}
	'drag' {
		$w = Get-Window
		Focus $w; MoveTo $w $X $Y
		[RzeWin]::mouse_event(0x2, 0, 0, 0, [UIntPtr]::Zero)
		for ($i = 1; $i -le 10; $i++) { MoveTo $w ($X + ($X2 - $X) * $i / 10) ($Y + ($Y2 - $Y) * $i / 10) }
		[RzeWin]::mouse_event(0x4, 0, 0, 0, [UIntPtr]::Zero)
		"dragged $X,$Y -> $X2,$Y2"
	}
	'key' {
		$w = Get-Window
		Focus $w
		[System.Windows.Forms.SendKeys]::SendWait($Keys)
		"sent $Keys"
	}
	'quit' {
		$proc = Get-Process Editor -ErrorAction SilentlyContinue
		if (-not $proc) { 'not running'; break }
		$proc | Stop-Process -Force -Confirm:$false
		# Stop-Process returns before the render thread / D3D device finish tearing down; poll until really gone
		$deadline = (Get-Date).AddSeconds(15)
		while ((Get-Process Editor -ErrorAction SilentlyContinue) -and (Get-Date) -lt $deadline) { Start-Sleep -Milliseconds 250 }
		if (Get-Process Editor -ErrorAction SilentlyContinue) { throw 'Editor.exe still running 15s after kill' }
		'quit'
	}
}
