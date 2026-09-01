#!/bin/sh
# Captures a running emulator's window to a PNG.
#
# # Why this is worth automating
#
# obSCEne draws its report to the screen, and that was built for the case where no text
# channel works (D030-ish reasoning: a black window cannot be told from a hung one). Two
# loaders are in exactly that case right now - Kyty and fpPS4 both run the module and emit
# no records at all - so **a screenshot may be the only report those loaders will ever
# give**, and capturing it is the difference between "0 records" and a result.
#
# It is also the cheapest possible evidence that a run did something, for a reader who is
# not going to read 800 pipe-separated lines.
#
# # Why this one file shells out to PowerShell
#
# Everything else in scripts/ is sh, deliberately (D071). Window capture is a Win32
# operation with no command-line equivalent, so this calls one inline PowerShell snippet
# the way other scripts call `multipass` - as a platform tool, not as an orchestration
# language. On anything else it reports that it cannot capture and exits zero, because a
# missing screenshot must never fail a run.
#
#   sh scripts/screenshot.sh --process shadPS4 --out reports/shots/shadps4.png
#   sh scripts/screenshot.sh --pid 1234 --out shot.png --delay 20
set -e

PROCESS=""
PID=""
OUT="reports/shots/capture.png"
DELAY=0

while [ $# -gt 0 ]; do
    case "$1" in
        --process) PROCESS="$2"; shift 2 ;;
        --pid) PID="$2"; shift 2 ;;
        --out) OUT="$2"; shift 2 ;;
        --delay) DELAY="$2"; shift 2 ;;
        *) echo "unknown option: $1" >&2; exit 2 ;;
    esac
done

if ! command -v powershell >/dev/null 2>&1; then
    echo "no window capture available on this platform; skipping" >&2
    exit 0
fi

mkdir -p "$(dirname "$OUT")"
target="$OUT"
command -v cygpath >/dev/null 2>&1 && target=$(cygpath -w "$OUT")

# PrintWindow rather than a screen grab of the desktop region.
#
# A screen grab captures whatever is on top, so another window or a screensaver silently
# becomes the evidence - which is worse than no screenshot, because it looks like one. Asking
# the window to render itself gets that window even when it is behind something.
#
# PW_RENDERFULLCONTENT (2) is needed for anything drawing through a GPU compositor, which
# every one of these emulators does; without it the capture is a blank rectangle of the
# right size, which is again a convincing-looking wrong answer.
powershell -NoProfile -NonInteractive -Command "
\$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
Add-Type @'
using System;
using System.Runtime.InteropServices;
public class ObsCap {
  [DllImport(\"user32.dll\")] public static extern bool PrintWindow(IntPtr h, IntPtr dc, uint f);
  [DllImport(\"user32.dll\")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
  [StructLayout(LayoutKind.Sequential)] public struct RECT { public int L, T, R, B; }
  public delegate bool EnumProc(IntPtr h, IntPtr p);
  [DllImport(\"user32.dll\")] public static extern bool EnumWindows(EnumProc f, IntPtr p);
  [DllImport(\"user32.dll\")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
  [DllImport(\"user32.dll\")] public static extern bool IsWindowVisible(IntPtr h);
  // MainWindowHandle is not the only way a process owns a window, and for some it is zero.
  //
  // .NET reports the first top-level window created by the *main thread* and caches it. A
  // loader that opens its window from a render thread, or after that value was first read,
  // has a perfectly good window and a MainWindowHandle of 0 - Kyty does exactly this, which
  // is why every capture of it came back \"no window to capture\" while the window was on
  // screen. Enumerating top-level windows and matching the owning process id finds it.
  // Visible is preferred and not required.
  //
  // Kyty's guest output is a correctly sized 1286x749 window that Win32 reports as *not
  // visible* - created hidden by its toolkit and never shown, because nobody is expected to
  // look at it. Requiring visibility rejected the one window worth capturing while three
  // zero- and one-pixel helper windows sat beside it. PrintWindow asks the window to render
  // itself, which is exactly why this script uses it over a screen grab, and works on a
  // window that was never put on screen.
  //
  // Size is the real filter: a toolkit leaves 0x0 and 1x1 windows lying about and those are
  // never the report.
  public static IntPtr FindWindow(uint want) {
    IntPtr visible = IntPtr.Zero, hidden = IntPtr.Zero;
    EnumWindows(delegate(IntPtr h, IntPtr p) {
      uint pid; GetWindowThreadProcessId(h, out pid);
      if (pid == want) {
        RECT r; GetWindowRect(h, out r);
        if (r.R - r.L > 64 && r.B - r.T > 64) {
          if (IsWindowVisible(h)) { visible = h; return false; }
          if (hidden == IntPtr.Zero) { hidden = h; }
        }
      }
      return true;
    }, IntPtr.Zero);
    return visible != IntPtr.Zero ? visible : hidden;
  }
}
'@
Start-Sleep -Seconds $DELAY
\$proc = \$null
if ('$PID' -ne '') { \$proc = Get-Process -Id $PID -ErrorAction SilentlyContinue }
elseif ('$PROCESS' -ne '') {
  # Not filtered on MainWindowHandle: a zero there means \"ask differently\", not \"no window\".
  \$proc = Get-Process -Name '$PROCESS' -ErrorAction SilentlyContinue | Select-Object -First 1
}
if (-not \$proc) { Write-Output 'no such process'; exit 0 }
\$h = \$proc.MainWindowHandle
if (\$h -eq 0) { \$h = [ObsCap]::FindWindow([uint32]\$proc.Id) }
if (\$h -eq 0) {
  Write-Output 'no window to capture'
  exit 0
}
\$r = New-Object ObsCap+RECT
[void][ObsCap]::GetWindowRect(\$h, [ref]\$r)
\$w = \$r.R - \$r.L; \$ht = \$r.B - \$r.T
if (\$w -le 0 -or \$ht -le 0) { Write-Output 'window has no size'; exit 0 }
\$bmp = New-Object System.Drawing.Bitmap \$w, \$ht
\$g = [System.Drawing.Graphics]::FromImage(\$bmp)
\$dc = \$g.GetHdc()
[void][ObsCap]::PrintWindow(\$h, \$dc, 2)
\$g.ReleaseHdc(\$dc)
\$bmp.Save('$target', [System.Drawing.Imaging.ImageFormat]::Png)
\$g.Dispose(); \$bmp.Dispose()
Write-Output (\"captured \" + \$w + \"x\" + \$ht)
" 2>/dev/null || echo "capture failed; continuing" >&2

if [ -f "$OUT" ]; then
    echo "screenshot: $OUT"
else
    echo "screenshot: none"
fi
