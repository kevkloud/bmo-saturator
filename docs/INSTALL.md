# BMO Saturator — installing

This build is **not code-signed**, so both operating systems will warn you the
first time. Getting past that is a couple of clicks and is described below.

## macOS

The zip contains:

| | where it goes |
|---|---|
| `BMO Saturator.vst3` | `/Library/Audio/Plug-Ins/VST3/` (or `~/Library/…` for just you) |
| `BMO Saturator.component` | `/Library/Audio/Plug-Ins/Components/` — Logic and GarageBand use this one |
| `BMO Saturator.app` | anywhere. It is the standalone, for a quick listen without a DAW |

`~/Library` is hidden in Finder: **Go → Go to Folder…** and paste the path.

**Gatekeeper.** Because nothing here is signed or notarised, macOS may say the
plugin "cannot be opened because the developer cannot be verified", or your DAW
may simply not list it. Two ways past:

- Open **System Settings → Privacy & Security**, scroll down, and click
  **Open Anyway** next to the message about BMO Saturator. Rescan in your DAW.
- Or, in Terminal, strip the download quarantine flag:

  ```bash
  xattr -dr com.apple.quarantine "/Library/Audio/Plug-Ins/VST3/BMO Saturator.vst3"
  xattr -dr com.apple.quarantine "/Library/Audio/Plug-Ins/Components/BMO Saturator.component"
  ```

Logic caches its plugin scan, so if the AU does not appear, quit Logic and
reopen it — and if it still refuses, run `auval -v aufx Bsat LT3a` in Terminal,
which will say what it objected to.

Both Apple Silicon and Intel are in the same binary.

## Windows

The zip contains `BMO Saturator.vst3` and `BMO Saturator.exe`, the standalone.
Copy the VST3 to:

```
C:\Program Files\Common Files\VST3\
```

The standalone can live anywhere.

**SmartScreen.** An unsigned download gets "Windows protected your PC" — click
**More info**, then **Run anyway**. If Windows has marked the zip itself as
blocked, right-click the zip → **Properties** → tick **Unblock** before
extracting, or the files inside inherit the block.

There is no VST2 build, and there will not be one.

## Both

Presets live in a folder you can open, copy from and back up:

- macOS: `~/Library/Audio/Presets/LT3 Audio/BMO Saturator`
- Windows: `%APPDATA%\LT3 Audio\BMO Saturator\Presets`

Sharing a preset is sending a file.

## If something is wrong

Please say what host, what OS, and what the plugin did, at
<https://github.com/kevkloud/bmo-saturator/issues>. "It sounds harsh on my
vocal" is a useful bug report for this one in particular — the whole plugin is
fitted to a specific measured target, and material that does not behave like
that target is exactly what needs finding.
