#!/bin/sh

READLINK="readlink"
SED="sed"
if [ "$(uname)" = "Darwin" ]
then
    if [ -z "$(which greadlink)" ] || [ -z "$(which gsed)" ]
    then
        echo "On MacOS, make sure you have GNU coreutils installed. Bailing out."
        exit 1
    fi
    READLINK="greadlink"
    SED="gsed"
fi

MYDIR=$(dirname $($READLINK -f "$0"))

cd "$MYDIR"/libressl
echo libressl-v3.0.1 > OPENBSD_BRANCH
$SED -i 's/git pull --rebase//' update.sh
./autogen.sh