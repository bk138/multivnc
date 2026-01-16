# 2. For build dependencies, use Conan for Desktop, keep submodules for Android

Date: 2026-01-16

## Status

Accepted

## Context

The integration of the [SSH tunneling feature for the Desktop version](https://github.com/bk138/multivnc/issues/200)
brought the question on how to organise the additional build dependencies. This was the starting point:

|               | Android               | Linux (Flatpak)        | MacOS              | Windows            |
| ------------- | --------------------- | ---------------------- | ------------------ | ------------------ |
| zlib          | from NDK              | from flatpak           | from MacOS         | -                  |
| libjpeg-turbo | `ExternalProject_Add` | from flatpak           | Homebrew           | -                  |
| Libre/OpenSSL | `ExternalProject_Add` | from flatpak (OpenSSL) | Homebrew           | -                  |
| libssh2       | `add_subdirectory`    | -                      | -                  | -                  |
| libsshtunnel  | `add_subdirectory`    | -                      | -                  | -                  |
| libvncclient  | `add_subdirectory`    | `add_subdirectory`     | `add_subdirectory` | `add_subdirectory` |
| wxWidgets     | N/A                   | via flatpak-build      | Homebrew           | downloaded binary  |
| libwxservdisc | N/A                   | `add_subdirectory`     | `add_subdirectory` | `add_subdirectory` |


The Android version was already using vendored-in dependencies as git submodules that are pulled in via CMake
`add_subdirectory` and `ExternalProject_Add`. It was tried to use this approach for the Desktop version as well,
but:

- this would need additional CMake logic to check with `find_package` first before using vendored-in version: 🆗
- it was found that getting all build dependencies for things like OpenSSL/LibreSSL is _really_  finnicky
  especially on Windows: 😒
- it would need to vendor-in otherwise ubiquitous stuff like zlib for Windows: 😒
- same approach as for Android version: 👍

Then, it was contemplated to use a C++ package manager like vcpkg or Conan to address the pain points of the
vendored-in dependecies approach:

- `find_package` can be used throughout: 👍
- no need for vendoring in stuff available in package managers: 👍
- would need custom recipes or fallback to externalproject again for stuff not readily available: 😐
- would mostly not re-use the git submodules from Android: 😐

## Decision

Use Conan for the Desktop version's dependencies _where applicable_, keep using vendored-in submodules for Android:

|               | Android               | Linux (Flatpak)        | MacOS              | Windows            |
| ------------- | --------------------- | ---------------------- | ------------------ | ------------------ |
| zlib          | from NDK              | from flatpak           | from MacOS         | Conan              |
| libjpeg-turbo | `ExternalProject_Add` | from flatpak           | Conan              | Conan              |
| Libre/OpenSSL | `ExternalProject_Add` | from flatpak (OpenSSL) | Conan              | Conan              |
| libssh2       | `add_subdirectory`    | via flatpak-build      | Conan              | Conan              |
| libsshtunnel  | `add_subdirectory`    | `add_subdirectory`     | `add_subdirectory` | `add_subdirectory` |
| libvncclient  | `add_subdirectory`    | `add_subdirectory`     | `add_subdirectory` | `add_subdirectory` |
| wxWidgets     | N/A                   | via flatpak-build      | Conan              | Conan              |
| libwxservdisc | N/A                   | `add_subdirectory`     | `add_subdirectory` | `add_subdirectory` |

That is:

- dependencies included via `add_subdirectory` are kept as-is (libvncclient, libwxservdisc)
- dependencies not available in Conan are vendored-in via simple `add_subdirectory` (libsshtunnel)
- Conan is used to pull in dependencies where the above is not done
- for the flatpak builds, flatpak-build YML is used instead of Conan, this can be revised at a later date 

Conan and vcpkg were both evaluated, with Conan found to have more ergnomic handling.

Both Conan and vcpkg were found to be pretty cumbersome for Android, so are not used there.

## Consequences

For the Android version, nothing changes.

For the Desktop versions, this means:

- `find_package` can be used throughout, allowing to use OS-provided dependencies _partially_ or _completely_
- there is no need for vendoring in dependencies available in package managers
- for demanding dependencies like Libre/OpenSSL, Conan takes care of building, making their use _much_ easier
- building does need fallback to vendoring-in for dependencies not readily available (libsshtunnel, libvncclient, libwxservdisc)
- building mostly does not re-use the git submodules from Android (only libsshtunnel, libvncclient are shared)
- there still is a difference in building flatpaks, which needs to be investigated at a later date

