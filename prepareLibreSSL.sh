#!/bin/sh

cd libressl
echo libressl-v3.0.1 > OPENBSD_BRANCH
sed -i 's/git pull --rebase//' update.sh
./autogen.sh