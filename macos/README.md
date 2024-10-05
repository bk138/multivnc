# MacOS Release Build Instructions

For the time being, we're building an Intel binary only. Universal builds to come later.

- Install build tools: `brew install cmake gettext`
- Install build dependencies: `brew install wxwidgets jpeg-turbo openssl`
- Build release app:
```
   mkdir build
   cd build
   MACOSX_DEPLOYMENT_TARGET=10.15 cmake .. -DCMAKE_BUILD_TYPE=Release
   cmake --build .
   cmake --install . --prefix .
```
