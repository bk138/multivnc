#!/bin/sh

[ -z "$CODESIGN_ID_DISTRIBUTION" ] && {
    echo "Please set CODESIGN_ID_DISTRIBUTION env var. You can get it via 'security find-identity -p codesigning -v'"
    exit 1
}

[ -z "$CODESIGN_ID_INSTALLER" ] && {
    echo "Please set CODESIGN_ID_INSTALLER env var. You can get it via 'security find-identity -v'"
    exit 1
}

[ -z "$APPLE_ID_EMAIL" ] && {
    echo "Please set APPLE_ID_EMAIL env var."
    exit 1
}

[ -z "$APPLE_ID_PASSWORD" ] && {
    echo "Please set APPLE_ID_PASSWORD env var.  Might need an app-specific password if using 2FA, which you can create at https://appstoreconnect.apple.com/apps"
    exit 1
}

# workaround until we're installing the whole framework https://github.com/bk138/multivnc/issues/244
[ -z "$WX_LOCALES_PATH" ] && {
    echo "Please set WX_LOCALES_PATH env var."
    exit 1
}

set -e

echo
echo "Build release app bundle"
echo
mkdir -p build-dir
cd build-dir
MACOSX_DEPLOYMENT_TARGET=10.15 cmake ../.. -DCMAKE_BUILD_TYPE=Release -DOPENSSL_ROOT_DIR=$(brew --prefix libressl)
make -j$(nproc)
cmake --install . --prefix .
# workaround until we're installing the whole framework https://github.com/bk138/multivnc/issues/244
cp $WX_LOCALES_PATH/de/LC_MESSAGES/wxstd-3.2.mo MultiVNC.app/Contents/Resources/de.lproj/
cp $WX_LOCALES_PATH/es/LC_MESSAGES/wxstd-3.2.mo MultiVNC.app/Contents/Resources/es.lproj/
cp $WX_LOCALES_PATH/sv/LC_MESSAGES/wxstd-3.2.mo MultiVNC.app/Contents/Resources/sv.lproj/

echo
echo "Sign embedded libs"
echo
codesign -s $CODESIGN_ID_DISTRIBUTION -f -i net.christianbeier.MultiVNC.libs MultiVNC.app/Contents/Frameworks/*

echo
echo "Sign app"
echo
codesign -s $CODESIGN_ID_DISTRIBUTION --entitlements ../MultiVNC.entitlements MultiVNC.app


echo
echo "Verify signing"
echo
codesign -d -vv MultiVNC.app

echo
echo "Build an installer package for App Store distribution"
echo
productbuild --sign $CODESIGN_ID_INSTALLER --component MultiVNC.app /Applications MultiVNC.pkg

echo
echo "Validate package"
echo
xcrun altool --validate-app -f MultiVNC.pkg -t osx -u $APPLE_ID_EMAIL -p $APPLE_ID_PASSWORD --output-format xml

if [ "$1" = "-u" ]
then
    echo
    echo "Upload package"
    echo
    xcrun altool --upload-app -f MultiVNC.pkg -t osx -u $APPLE_ID_EMAIL -p $APPLE_ID_PASSWORD --output-format xml
else
    echo
    echo "Provide -u to upload package"
    echo
fi
