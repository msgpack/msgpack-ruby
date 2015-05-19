#!/bin/bash

RUBY_PATH=$1
RUBY_VERSIONS=$2

export PATH=$RUBY_PATH/bin:$PATH

git remote add onetime $MSGPACK_REPO
git checkout master
git pull onetime $BUILD_BRANCH
git reset --hard $BUILD_POSITION
bundle install
rake cross native gem RUBY_CC_VERSION=$RUBY_VERSIONS
