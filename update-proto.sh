#!/bin/sh

# Name of the temporary directory to clone steamkit into
TMP_NAME=proto-tmp

# Steamkit repository
PROTO_REPO=https://github.com/SteamRE/SteamKit.git

# Protobuf path
PROTO_PATH=Resources/Protobufs/dota/

git init $TMP_NAME
cd $TMP_NAME
git remote add origin $PROTO_REPO
git config core.sparsecheckout true
echo $PROTO_PATH > .git/info/sparse-checkout
git pull origin master
rm ../proto/*.proto
cp ${PROTO_PATH}*.proto ../proto/
cd ..
rm -Rf $TMP_NAME
