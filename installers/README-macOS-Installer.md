zconverter — macOS installer build instructions

Prerequisites (on the mac build machine):
- Qt 6 installed for macOS
- CMake 3.28+
- Xcode command line tools
- `macdeployqt` available in PATH
- `hdiutil` (built into macOS)
- Optional: `LibreOffice.app` if you want to bundle LibreOffice into the app bundle

What this creates
- A signed-unless-you-sign-it-yourself `.app` bundle with Qt frameworks deployed
- An optional bundled `LibreOffice.app` inside `zconverter.app/Contents/Frameworks`
- A distributable `.dmg` setup image

Quick start

```bash
chmod +x installers/create_macos_dmg.sh
./installers/create_macos_dmg.sh \
  --preset mac-release \
  --build-dir out/build/macos/mac-release \
  --install-dir out/install/macos/mac-release \
  --output out/dist/zconverter-setup.dmg \
  --libreoffice /Applications/LibreOffice.app
```

If you already have LibreOffice installed in `/Applications/LibreOffice.app`, you can omit the `--libreoffice` flag because the script will use that by default.

How it works
- The CMake install step writes `zconverter.app` into the install directory.
- `macdeployqt` copies the Qt runtime frameworks and plugins into the app bundle.
- The script copies `LibreOffice.app` into `zconverter.app/Contents/Frameworks/LibreOffice.app` when provided.
- The final DMG contains the app and an `/Applications` shortcut.

If you prefer a manual flow:

```bash
cmake --preset mac-release
cmake --build --preset mac-release --config Release --target install
macdeployqt out/install/macos/mac-release/zconverter.app -always-overwrite
hdiutil create -volname zconverter -srcfolder out/install/macos/mac-release -ov -format UDZO out/dist/zconverter-setup.dmg
```

Notes
- This bundles Qt automatically. LibreOffice is optional because it is large; pass a path to a local `LibreOffice.app` if you want it embedded.
- If you plan to distribute the DMG publicly, consider code-signing and notarization before shipping.
