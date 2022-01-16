#!/bin/sh

usage() {
    echo "This scripts clones your private git repository containing your"
    echo "Lorawan keys."
    echo
    echo "Usage:"
    echo " $0 <git repository URL>"
}

#Clone keys repo
GIT_REPO=$1 #Something like ssh://git@git.example.com/angelnu/lorawan-keys.git

cd "$(dirname "$0")"
if [ -d config ]; then
    echo "config folder already exists - terminating"
    exit 1
fi

if [ ! -n "$GIT_REPO" ]; then
    usage
    exit 2
fi

echo "Cloning $GIT_REPO"
echo git clone "$GIT_REPO" config