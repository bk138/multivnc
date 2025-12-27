# MacOS Release Build Instructions

For the time being, we're building an Intel binary only. Universal builds to come later.

- Install build tools: `brew install cmake gettext nasm`
- Install build dependencies: `brew install wxwidgets libressl`
- Building a release app bundle, [signing the app](https://developer.apple.com/documentation/xcode/creating-distribution-signed-code-for-the-mac),
  [building an installer package for App Store distribution](https://developer.apple.com/documentation/xcode/packaging-mac-software-for-distribution) as well as
  [validating and uploading the package](https://help.apple.com/asc/appsaltool) is all done by the `build-sign-validate-upload.sh` script.
