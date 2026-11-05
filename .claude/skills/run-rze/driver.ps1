<#
Drives the RZE Editor (native Win32 + DX11 + Dear ImGui) with real mouse/keyboard input.
One action per invocation; state lives in the running Editor.exe process.

  driver.ps1 launch [-Scene DrawLineTest.scene] [-Config debug] [-SettleSeconds 5] [-KeepImguiIni]
                                               # backs up RZE\Config\imgui.ini, which close and quit put back
  driver.ps1 shot   [-Out <path.png>]          # client-area screenshot; default $env:TEMP\rze-shots\<timestamp>.png
  driver.ps1 click  -X <int> -Y <int>          # client-area pixel coords (same space as screenshots)
  driver.ps1 drag   -X <int> -Y <int> -X2 <int> -Y2 <int>
  driver.ps1 key    -Keys '<SendKeys string>'  # e.g. '^a45{ENTER}' to replace an ImGui float field
  driver.ps1 info                              # window title + client size
  driver.ps1 alive                             # running / responding?
  driver.ps1 close  [-TimeoutSeconds 30]       # closes the window like a user would, so the editor shuts down normally;
                                               # fails if it crashes on the way out or is still open after the timeout
  driver.ps1 quit                              # kills Editor.exe (unsaved scene edits are discarded)
  driver.ps1 scroll -X <int> -Y <int> -Notches <int>   # wheel notches at a point; + is away from you (zooms in)

With launch -Harness, the session runs EditorHarness.exe instead: input, screenshots and questions go through the app
itself (JSON lines on its stdin), so the real cursor and focus are left alone and the window may be covered. Every
command above goes through it, and these work only there:
  driver.ps1 state                             # scene, modified, selection, undo, objects
  driver.ps1 object -Name <name>               # one object's GUID, parent, components and transform
  driver.ps1 realinput -On on|off              # let the real mouse and keyboard through, or block them (the default)
#>
param(
	[Parameter(Mandatory, Position = 0)]
	[ValidateSet('launch', 'shot', 'click', 'dblclick', 'drag', 'scroll', 'key', 'info', 'alive', 'close', 'quit', 'state', 'object', 'realinput', 'helper')]
	[string] $Action,
	[string] $Scene = 'DrawLineTest.scene',
	[ValidateSet('debug', 'release')] [string] $Config = 'debug',
	[int] $SettleSeconds = 5,
	# launch: how long to wait for the window. close: how long to wait for the process to exit (30 unless given).
	[int] $TimeoutSeconds = 90,
	[string] $Out,
	[int] $X, [int] $Y,
	[int] $X2, [int] $Y2,
	[string] $Keys,
	# launch: keep the layout the run leaves, instead of putting imgui.ini back as it was
	[switch] $KeepImguiIni,
	# launch: run EditorHarness.exe, and drive it through its stdin
	[switch] $Harness,
	[int] $Notches,
	[string] $Name,
	[ValidateSet('on', 'off')] [string] $On
)

$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path
$buildDir = Join-Path $repoRoot "RZE\_build\$Config"

# Every Editor run rewrites imgui.ini, which often holds a layout the user means to keep (CLAUDE.md). launch copies it
# here, and close and quit put it back once the Editor has exited, as driver.py does. A file, since each call is a
# process of its own; one a session left (it died before close or quit) is put back by the next launch.
$imguiIni = Join-Path $repoRoot 'RZE\Config\imgui.ini'
$imguiIniBackup = Join-Path $env:TEMP 'rze-driver\imgui.ini.backup'

function Restore-ImguiIni {
	if (-not (Test-Path $imguiIniBackup)) { return $false }
	Copy-Item $imguiIniBackup $imguiIni -Force
	# A copy keeps the backup's date; a new one, so nothing mistakes it for the old file
	(Get-Item $imguiIni).LastWriteTime = Get-Date
	Remove-Item $imguiIniBackup -Force
	return $true
}

function Backup-ImguiIni {
	$restored = Restore-ImguiIni
	if (Test-Path $imguiIni) {
		New-Item -ItemType Directory -Force (Split-Path $imguiIniBackup) | Out-Null
		Copy-Item $imguiIni $imguiIniBackup -Force
	}
	return $restored
}

function Get-RestoreNote {
	if (Restore-ImguiIni) { '; imgui.ini restored' } else { '' }
}

# ---------------------------------------------------------------------------------------------------------------------
# Harness sessions. Each call is a process of its own, so a hidden helper (driver.ps1 helper) holds EditorHarness's
# stdin and stdout for the session. Calls reach it through a named pipe, one request line and one answer line each.

$harnessStateFile = Join-Path $env:TEMP 'rze-driver\harness-session.json'
$harnessPipeName = 'rze-driver-harness'
$harnessAppLog = Join-Path $env:TEMP 'rze-driver\harness-app.log'

$harnessHelperSource = @'
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.IO.Pipes;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading;

// Owns EditorHarness.exe for a session. A thread reads its stdout: "RZEHARNESS {json}" answers are kept by id, and
// everything else goes to the app log. Requests come in on a named pipe, one line each:
//   <seconds> {json}   a harness command, sent with the next id; answered with the harness's reply
//   @ready <seconds>   waits for the harness's ready event
//   @alive             whether it's running, and its exit code once it isn't
//   @wait-exit <s>     waits for it to exit
//   @stop              kills it if it's still running, and ends the helper
public static class RzeHarnessHelper
{
    static Process s_process;
    static StreamWriter s_stdin;
    static readonly object s_gate = new object();
    static readonly Dictionary<long, string> s_replies = new Dictionary<long, string>();
    static bool s_isReady;
    static long s_nextId;
    static readonly Regex s_id = new Regex("^\\{\"id\":(-?\\d+)");
    const string k_prefix = "RZEHARNESS ";

    public static int Run(string exe, string arguments, string workingDirectory, string pipeName, string logPath)
    {
        // Without this, Process.Start writes a UTF-8 byte order mark to the harness's stdin
        Console.InputEncoding = new UTF8Encoding(false);
        ProcessStartInfo info = new ProcessStartInfo(exe, arguments);
        info.WorkingDirectory = workingDirectory;
        info.UseShellExecute = false;
        info.CreateNoWindow = true;
        info.RedirectStandardInput = true;
        info.RedirectStandardOutput = true;
        info.StandardOutputEncoding = new UTF8Encoding(false);
        s_process = Process.Start(info);
        s_stdin = s_process.StandardInput;
        s_stdin.AutoFlush = true;

        Thread reader = new Thread(() => ReadOutput(logPath));
        reader.IsBackground = true;
        reader.Start();

        try { return Serve(pipeName); }
        finally
        {
            // However the helper ends, it doesn't leave the harness running with nobody to drive or close it
            if (!s_process.HasExited) { s_process.Kill(); s_process.WaitForExit(15000); }
        }
    }

    static int Serve(string pipeName)
    {
        DateTime exitedAt = DateTime.MaxValue;
        while (true)
        {
            using (NamedPipeServerStream server = new NamedPipeServerStream(pipeName, PipeDirection.InOut, 1, PipeTransmissionMode.Byte, PipeOptions.Asynchronous))
            {
                IAsyncResult waiting = server.BeginWaitForConnection(null, null);
                // Once the harness is gone, a helper nobody asks about for a minute stops by itself
                while (!waiting.AsyncWaitHandle.WaitOne(1000))
                {
                    if (s_process.HasExited && exitedAt == DateTime.MaxValue) { exitedAt = DateTime.Now; }
                    if ((DateTime.Now - exitedAt).TotalSeconds > 60) { return 0; }
                }
                server.EndWaitForConnection(waiting);
                StreamReader request = new StreamReader(server, new UTF8Encoding(false));
                StreamWriter answer = new StreamWriter(server, new UTF8Encoding(false));
                answer.AutoFlush = true;
                string line = request.ReadLine() ?? "";
                bool stop = line.StartsWith("@stop");
                string reply;
                try { reply = Handle(line); }
                catch (Exception e) { reply = "{\"ok\":false,\"error\":" + Quote("helper: " + e.Message) + "}"; }
                answer.WriteLine(reply);
                server.WaitForPipeDrain();
                if (stop) { return 0; }
            }
        }
    }

    static string Handle(string line)
    {
        if (line.StartsWith("@ready"))
        {
            DateTime deadline = DateTime.Now.AddSeconds(Seconds(line));
            lock (s_gate)
            {
                while (!s_isReady && !s_process.HasExited && DateTime.Now < deadline) { Monitor.Wait(s_gate, 200); }
                return s_isReady ? "{\"ok\":true}" : "{\"ok\":false,\"error\":\"the harness didn't get ready" + (s_process.HasExited ? " (it exited)" : "") + "\"}";
            }
        }
        if (line.StartsWith("@alive")) { return Alive(); }
        if (line.StartsWith("@wait-exit"))
        {
            s_process.WaitForExit((int)(Seconds(line) * 1000));
            return Alive();
        }
        if (line.StartsWith("@stop"))
        {
            if (!s_process.HasExited) { s_process.Kill(); s_process.WaitForExit(15000); }
            return Alive();
        }

        int space = line.IndexOf(' ');
        double timeout = double.Parse(line.Substring(0, space), System.Globalization.CultureInfo.InvariantCulture);
        string json = line.Substring(space + 1).Trim();
        if (s_process.HasExited) { return "{\"ok\":false,\"error\":\"the app has exited (code " + s_process.ExitCode + ")\"}"; }
        long id;
        lock (s_gate) { id = ++s_nextId; }
        // The id goes first, as the harness's own answers have it
        s_stdin.WriteLine("{\"id\":" + id + (json.Length > 2 ? "," + json.Substring(1) : "}"));
        DateTime until = DateTime.Now.AddSeconds(timeout);
        lock (s_gate)
        {
            while (!s_replies.ContainsKey(id))
            {
                if (s_process.HasExited) { return "{\"ok\":false,\"exited\":true,\"error\":\"no answer: the app exited\"}"; }
                if (DateTime.Now > until) { return "{\"ok\":false,\"error\":\"no answer from the harness after " + timeout + " s\"}"; }
                Monitor.Wait(s_gate, 100);
            }
            string reply = s_replies[id];
            s_replies.Remove(id);
            return reply;
        }
    }

    static void ReadOutput(string logPath)
    {
        using (StreamWriter log = new StreamWriter(logPath, false, new UTF8Encoding(false)))
        {
            log.AutoFlush = true;
            string line;
            while ((line = s_process.StandardOutput.ReadLine()) != null)
            {
                if (!line.StartsWith(k_prefix)) { log.WriteLine(line); continue; }
                string json = line.Substring(k_prefix.Length);
                lock (s_gate)
                {
                    Match id = s_id.Match(json);
                    if (id.Success) { s_replies[long.Parse(id.Groups[1].Value)] = json; }
                    else if (json.Contains("\"event\":\"ready\"")) { s_isReady = true; }
                    else { log.WriteLine(line); }
                    Monitor.PulseAll(s_gate);
                }
            }
        }
        lock (s_gate) { Monitor.PulseAll(s_gate); }
    }

    static string Alive()
    {
        return s_process.HasExited
            ? "{\"ok\":true,\"running\":false,\"pid\":" + s_process.Id + ",\"code\":" + s_process.ExitCode + "}"
            : "{\"ok\":true,\"running\":true,\"pid\":" + s_process.Id + "}";
    }

    static double Seconds(string line)
    {
        string[] parts = line.Split(' ');
        return parts.Length > 1 ? double.Parse(parts[1], System.Globalization.CultureInfo.InvariantCulture) : 30;
    }

    static string Quote(string text)
    {
        return "\"" + text.Replace("\\", "\\\\").Replace("\"", "\\\"") + "\"";
    }
}
'@

# Whether an Editor or EditorHarness is running. One that has just exited is still listed for a moment, so a launch
# right after a close waits for it to go rather than refusing.
function Test-EditorRunning([string[]]$names) {
	$deadline = (Get-Date).AddSeconds(5)
	while (Get-Process $names -ErrorAction SilentlyContinue) {
		if ((Get-Date) -gt $deadline) { return $true }
		Start-Sleep -Milliseconds 250
	}
	return $false
}

# The session, or nothing if there's none (or its helper has died, which ends it)
function Get-HarnessSession {
	if (-not (Test-Path $harnessStateFile)) { return $null }
	$session = Get-Content $harnessStateFile -Raw | ConvertFrom-Json
	if (-not (Get-Process -Id $session.HelperPid -ErrorAction SilentlyContinue)) {
		Remove-Item $harnessStateFile -Force
		return $null
	}
	return $session
}

# One request to the helper; the answer's text
function Send-HelperLine([string]$line, [int]$connectTimeoutMs = 5000) {
	$client = New-Object System.IO.Pipes.NamedPipeClientStream('.', $harnessPipeName, [System.IO.Pipes.PipeDirection]::InOut)
	try {
		$client.Connect($connectTimeoutMs)
		$utf8 = New-Object System.Text.UTF8Encoding($false)
		$writer = New-Object System.IO.StreamWriter($client, $utf8); $writer.AutoFlush = $true
		$reader = New-Object System.IO.StreamReader($client, $utf8)
		$writer.WriteLine($line)
		return $reader.ReadLine()
	}
	finally { $client.Dispose() }
}

# A harness command (a hashtable without its id); its answer as an object. Throws with the harness's error if it failed.
function Invoke-Harness([hashtable]$command, [int]$timeoutSec = 30) {
	$answer = Send-HelperLine ("$timeoutSec " + ($command | ConvertTo-Json -Compress)) | ConvertFrom-Json
	# An input can end the app before it's answered, e.g. a click on the prompt's Don't Save while exiting
	if (-not $answer.ok -and $answer.exited) { return [pscustomobject]@{ ok = $true; exited = $true } }
	if (-not $answer.ok) { throw "harness: $($answer.error)" }
	return $answer
}

function Get-ExitNote($reply) { if ($reply.exited) { '; the app exited' } else { '' } }

function Get-HarnessAlive { Send-HelperLine '@alive' | ConvertFrom-Json }

# Ends the session: the helper stops (killing the harness if it's still running), and the layout's put back
function Stop-HarnessSession {
	$alive = Send-HelperLine '@stop' | ConvertFrom-Json
	Remove-Item $harnessStateFile -Force -ErrorAction SilentlyContinue
	return $alive
}

function Get-DefaultShotPath {
	$dir = Join-Path $env:TEMP 'rze-shots'
	New-Item -ItemType Directory -Force $dir | Out-Null
	Join-Path $dir ("{0:yyyyMMdd-HHmmss-fff}.png" -f (Get-Date))
}

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
	[DllImport("user32.dll")] public static extern bool PostMessage(IntPtr h, uint m, IntPtr w, IntPtr l);
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

# The hidden helper of a harness session (launch -Harness starts it); runs until the session ends
if ($Action -eq 'helper') {
	Add-Type -TypeDefinition $harnessHelperSource -ReferencedAssemblies System.Core
	exit [RzeHarnessHelper]::Run((Join-Path $buildDir 'EditorHarness.exe'), "-scene $Scene", $buildDir, $harnessPipeName, $harnessAppLog)
}

if ($Action -eq 'launch' -and $Harness) {
	if (Get-HarnessSession) { throw 'A harness session is running. Run: driver.ps1 close (or quit)' }
	if (Test-EditorRunning Editor, EditorHarness) { throw 'Editor.exe or EditorHarness.exe already running. Run: driver.ps1 quit' }
	$exe = Join-Path $buildDir 'EditorHarness.exe'
	if (-not (Test-Path $exe)) { throw "Not built: $exe" }
	$restoredLeftover = if ($KeepImguiIni) { Restore-ImguiIni } else { Backup-ImguiIni }
	New-Item -ItemType Directory -Force (Split-Path $harnessStateFile) | Out-Null

	$helper = Start-Process powershell.exe -WindowStyle Hidden -PassThru -ArgumentList @(
		'-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', "`"$PSCommandPath`"", 'helper', '-Config', $Config, '-Scene', $Scene)
	# Up once its pipe answers
	$alive = $null
	$deadline = (Get-Date).AddSeconds(30)
	while (-not $alive) {
		if ($helper.HasExited) { throw "The harness helper exited at startup (code $($helper.ExitCode))$(Get-RestoreNote)" }
		if ((Get-Date) -gt $deadline) { throw 'The harness helper never answered' }
		try { $alive = Get-HarnessAlive } catch { Start-Sleep -Milliseconds 300 }
	}
	@{ HelperPid = $helper.Id; HarnessPid = $alive.pid; Config = $Config } | ConvertTo-Json | Set-Content $harnessStateFile -Encoding UTF8

	$ready = Send-HelperLine "@ready $TimeoutSeconds" | ConvertFrom-Json
	if (-not $ready.ok) {
		$null = Stop-HarnessSession
		throw "$($ready.error); its output is in $harnessAppLog$(Get-RestoreNote)"
	}
	# The Editor maximizes itself once it's up; coordinates aren't final until it has
	$deadline = (Get-Date).AddSeconds(15)
	do { $info = Invoke-Harness @{ cmd = 'info' }; if (-not $info.maximized) { Start-Sleep -Milliseconds 100 } }
	while (-not $info.maximized -and (Get-Date) -lt $deadline)
	# Meshes and textures stream in after the title's set
	Start-Sleep -Seconds $SettleSeconds
	$note = if ($KeepImguiIni) { $(if ($restoredLeftover) { ' (restored an imgui.ini backup a session left; this run keeps its layout)' } else { '' }) }
		else { " (imgui.ini backed up$(if ($restoredLeftover) { '; restored a backup a session left' }))" }
	"launched harness pid=$($alive.pid) title='$($info.title)' client=$($info.width)x$($info.height)$(if (-not $info.maximized) { ' (not maximized)' })$note"
	exit 0
}

# While a harness session runs, every command goes through the harness
$session = Get-HarnessSession
if ($session) {
	switch ($Action) {
		'launch' { throw 'A harness session is running. Run: driver.ps1 close (or quit)' }
		'click' { $reply = Invoke-Harness @{ cmd = 'click'; x = $X; y = $Y }; "clicked $X,$Y (harness$(Get-ExitNote $reply))" }
		'dblclick' { $reply = Invoke-Harness @{ cmd = 'dblclick'; x = $X; y = $Y }; "double-clicked $X,$Y (harness$(Get-ExitNote $reply))" }
		'drag' { $reply = Invoke-Harness @{ cmd = 'drag'; x = $X; y = $Y; x2 = $X2; y2 = $Y2 }; "dragged $X,$Y -> $X2,$Y2 (harness$(Get-ExitNote $reply))" }
		'scroll' { $reply = Invoke-Harness @{ cmd = 'scroll'; x = $X; y = $Y; notches = $Notches }; "scrolled $Notches at $X,$Y (harness$(Get-ExitNote $reply))" }
		'key' { $reply = Invoke-Harness @{ cmd = 'key'; keys = $Keys }; "sent $Keys (harness$(Get-ExitNote $reply))" }
		'shot' {
			if (-not $Out) { $Out = Get-DefaultShotPath }
			$reply = Invoke-Harness @{ cmd = 'shot'; path = $Out } 60
			"saved $($reply.path) ($($reply.width)x$($reply.height); the frame itself, from the harness)"
		}
		'info' {
			$info = Invoke-Harness @{ cmd = 'info' }
			"title='$($info.title)' client=$($info.width)x$($info.height) frame=$($info.frame) (harness; real input $(if ($info.realInputBlocked) { 'blocked' } else { 'allowed' }))"
		}
		'alive' {
			$alive = Get-HarnessAlive
			if ($alive.running) { "running pid=$($alive.pid) (harness)" } else { "exited code=$($alive.code) (harness)" }
		}
		{ $_ -in 'state', 'object' } {
			$reply = if ($Action -eq 'state') { Invoke-Harness @{ cmd = 'state' } } else { Invoke-Harness @{ cmd = 'object'; name = $Name } }
			$reply.PSObject.Properties.Remove('id'); $reply.PSObject.Properties.Remove('ok')
			$reply | ConvertTo-Json -Depth 8
		}
		'realinput' {
			if (-not $On) { throw 'realinput needs -On on or -On off' }
			$null = Invoke-Harness @{ cmd = 'realinput'; on = ($On -eq 'on') }
			"real input $(if ($On -eq 'on') { 'allowed' } else { 'blocked' }) (harness)"
		}
		'close' {
			$timeout = if ($PSBoundParameters.ContainsKey('TimeoutSeconds')) { $TimeoutSeconds } else { 30 }
			# As the window's close button does: unsaved changes are asked about first. Nothing to send if it has already
			# exited (the prompt's Don't Save, say).
			$alive = Get-HarnessAlive
			if ($alive.running) {
				$null = Invoke-Harness @{ cmd = 'quit' }
				$alive = Send-HelperLine "@wait-exit $timeout" | ConvertFrom-Json
			}
			if ($alive.running) {
				# Left open so it can be looked at and answered, as a person would
				$title = (Invoke-Harness @{ cmd = 'info' }).title
				throw "EditorHarness still running ${timeout}s after close (title: '$title'). An unsaved-changes prompt? Run: driver.ps1 shot, then click Don't Save (driver.ps1 click) and close again, or driver.ps1 quit"
			}
			$null = Stop-HarnessSession
			$restoreNote = Get-RestoreNote
			if ($alive.code -ne 0) { throw ("EditorHarness exited with code 0x{0:X8} while shutting down{1}" -f $alive.code, $restoreNote) }
			"closed code=0 (harness)$restoreNote"
		}
		'quit' { $null = Stop-HarnessSession; "quit (harness)$(Get-RestoreNote)" }
	}
	exit 0
}
if ($Action -in 'state', 'object', 'realinput') { throw "$Action works in harness sessions only: driver.ps1 launch -Harness" }

switch ($Action) {
	'launch' {
		if (Test-EditorRunning Editor, EditorHarness) { throw 'Editor.exe or EditorHarness.exe already running. Run: driver.ps1 quit' }
		$exe = Join-Path $buildDir 'Editor.exe'
		if (-not (Test-Path $exe)) { throw "Not built: $exe" }
		# With -KeepImguiIni nothing's backed up, but a backup a dead session left is still put back first
		$restoredLeftover = if ($KeepImguiIni) { Restore-ImguiIni } else { Backup-ImguiIni }
		# Assets resolve against the source tree from the exe's path, not the working directory; the build dir is just a tidy cwd
		$p = Start-Process -FilePath $exe -ArgumentList "-scene $Scene" -WorkingDirectory $buildDir -PassThru
		$deadline = (Get-Date).AddSeconds($TimeoutSeconds)
		while ((Get-Date) -lt $deadline) {
			Start-Sleep -Milliseconds 500
			$p.Refresh()
			if ($p.HasExited) { throw "Editor.exe exited during startup (code $($p.ExitCode))$(Get-RestoreNote)" }
			if ($p.MainWindowTitle -like "*$Scene") { break }
		}
		if ($p.MainWindowTitle -notlike "*$Scene") { throw "Timed out waiting for window title to show $Scene (title: '$($p.MainWindowTitle)')" }
		# Title is set when the load starts; meshes/textures stream in over the following frames
		Start-Sleep -Seconds $SettleSeconds
		$w = Get-Window
		$note = if ($KeepImguiIni) { $(if ($restoredLeftover) { ' (restored an imgui.ini backup a session left; this run keeps its layout)' } else { '' }) }
			else { " (imgui.ini backed up$(if ($restoredLeftover) { '; restored a backup a session left' }))" }
		"launched pid=$($p.Id) title='$($p.MainWindowTitle)' client=$($w.Width)x$($w.Height)$note"
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
	'dblclick' {
		# Both clicks from one process: two separate `click` calls are too far apart to count as a double-click
		$w = Get-Window
		Focus $w; MoveTo $w $X $Y
		for ($i = 0; $i -lt 2; $i++) {
			[RzeWin]::mouse_event(0x2, 0, 0, 0, [UIntPtr]::Zero); Start-Sleep -Milliseconds 30
			[RzeWin]::mouse_event(0x4, 0, 0, 0, [UIntPtr]::Zero); Start-Sleep -Milliseconds 60
		}
		"double-clicked $X,$Y"
	}
	'drag' {
		$w = Get-Window
		Focus $w; MoveTo $w $X $Y
		[RzeWin]::mouse_event(0x2, 0, 0, 0, [UIntPtr]::Zero)
		for ($i = 1; $i -le 10; $i++) { MoveTo $w ($X + ($X2 - $X) * $i / 10) ($Y + ($Y2 - $Y) * $i / 10) }
		[RzeWin]::mouse_event(0x4, 0, 0, 0, [UIntPtr]::Zero)
		"dragged $X,$Y -> $X2,$Y2"
	}
	'scroll' {
		$w = Get-Window
		Focus $w; MoveTo $w $X $Y
		# MOUSEEVENTF_WHEEL, 120 a notch; the delta is a signed value in an unsigned field
		$delta = [BitConverter]::ToUInt32([BitConverter]::GetBytes([int](120 * $Notches)), 0)
		[RzeWin]::mouse_event(0x0800, 0, 0, $delta, [UIntPtr]::Zero); Start-Sleep -Milliseconds 100
		"scrolled $Notches at $X,$Y"
	}
	'key' {
		$w = Get-Window
		Focus $w
		[System.Windows.Forms.SendKeys]::SendWait($Keys)
		"sent $Keys"
	}
	'close' {
		$w = Get-Window
		$proc = $w.Proc
		# Opens a handle now, so the exit code can still be read once the process is gone
		$null = $proc.Handle
		$timeout = if ($PSBoundParameters.ContainsKey('TimeoutSeconds')) { $TimeoutSeconds } else { 30 }

		# WM_CLOSE: the same path as the window's close button, so the engine runs its whole shutdown
		[RzeWin]::PostMessage($w.Handle, 0x0010, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null
		if (-not $proc.WaitForExit($timeout * 1000)) {
			# Left open so it can be looked at: an unsaved-changes prompt, a Debug assert dialog, or a hang. The layout's
			# put back once it has exited (close again, or quit), since it rewrites imgui.ini as it shuts down.
			throw "Editor.exe still running ${timeout}s after close (title: '$((Get-EditorProcess).MainWindowTitle)'). Run: driver.ps1 shot to see why, then answer the prompt or driver.ps1 quit"
		}
		$restoreNote = Get-RestoreNote
		if ($proc.ExitCode -ne 0) {
			throw ("Editor.exe exited with code 0x{0:X8} while shutting down{1}" -f $proc.ExitCode, $restoreNote)
		}
		"closed code=0$restoreNote"
	}
	'quit' {
		$proc = Get-Process Editor -ErrorAction SilentlyContinue
		if (-not $proc) { "not running$(Get-RestoreNote)"; break }
		$proc | Stop-Process -Force -Confirm:$false
		# Stop-Process returns before the render thread / D3D device finish tearing down; poll until really gone
		$deadline = (Get-Date).AddSeconds(15)
		while ((Get-Process Editor -ErrorAction SilentlyContinue) -and (Get-Date) -lt $deadline) { Start-Sleep -Milliseconds 250 }
		if (Get-Process Editor -ErrorAction SilentlyContinue) { throw 'Editor.exe still running 15s after kill' }
		"quit$(Get-RestoreNote)"
	}
}
