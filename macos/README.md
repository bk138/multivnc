# MacOS Release Build Instructions

For the time being, we're building an Intel binary only. Universal builds to come later.

- Install build tools: `brew install cmake gettext`
- Install build dependencies: `brew install wxwidgets jpeg-turbo openssl`
- Build release app bundle:
```
   mkdir build
   cd build
   MACOSX_DEPLOYMENT_TARGET=10.15 cmake .. -DCMAKE_BUILD_TYPE=Release
   cmake --build .
   cmake --install . --prefix .
```
- [Sign the app](https://developer.apple.com/documentation/xcode/creating-distribution-signed-code-for-the-mac):
  - Get distribution codesigning identity: `security find-identity -p codesigning -v`
  - Sign embedded libs: `codesign -s <codesigning-ID> -f -i net.christianbeier.MultiVNC.libs MultiVNC.app/Contents/Frameworks/*`
  - Sign app: `codesign -s <codesigning-ID> --entitlements ../macos/MultiVNC.entitlements MultiVNC.app`
  - Verify: `codesign -d -vv MultiVNC.app`
