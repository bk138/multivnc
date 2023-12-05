#!/bin/sh

READLINK="readlink"
if [ "$(uname)" = "Darwin" ]
then
    if [ -z "$(which greadlink)" ]
    then
        echo "On MacOS, make sure you have GNU coreutils installed. Bailing out."
        exit 1
    fi
    READLINK="greadlink"
fi

MYDIR=$(dirname $($READLINK -f "$0"))

cd "$MYDIR"/libressl
./autogen.sh