# MacOS Release Build Instructions

For the time being, we're building an Intel binary only. Universal builds to come later.

- Install Conan: `brew install conan`
- Setup Conan: `conan profile detect --force`
- Building a release app bundle, [signing the app](https://developer.apple.com/documentation/xcode/creating-distribution-signed-code-for-the-mac),
  [building an installer package for App Store distribution](https://developer.apple.com/documentation/xcode/packaging-mac-software-for-distribution) as well as
  [validating and uploading the package](https://help.apple.com/asc/appsaltool) is all done by the `build-sign-validate-upload.sh` script.
