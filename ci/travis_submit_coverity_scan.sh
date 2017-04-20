#!/bin/bash

set -ex

# Skip if branch is not coverity_scan
[ "$TRAVIS_BRANCH" != "coverity_scan" ] && exit 0

bundle exec rake clean
rm -rf cov-int
cov-build --dir cov-int bundle exec rake compile
tar czvf msgpack-ruby-coverity.tar.gz cov-int

set +x  # not to leak COVERITY_SCAN_TOKEN to stdout

echo "Posting to https://scan.coverity.com/builds?project=msgpack%2Fmsgpack-ruby"

curl --form token="$COVERITY_SCAN_TOKEN" \
  --form email=frsyuki@gmail.com \
  --form file=@msgpack-ruby-coverity.tar.gz \
  --form version="$(git show --pretty=format:'%H-%ad' --date 'format:%Y%m%dT%H%M%S' $TRAVIS_COMMIT | head -n 1)" \
  --form description="msgpack-ruby coverity scan" \
  "https://scan.coverity.com/builds?project=msgpack%2Fmsgpack-ruby"

