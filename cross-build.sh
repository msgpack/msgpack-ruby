#!/bin/bash

cd /home/ubuntu/msgpack-ruby
git remote add onetime $MSGPACK_REPO
git checkout master
git pull onetime $BUILD_BRANCH
git reset --hard $BUILD_POSITION
bundle install
bundle rake cross native gem
