FROM ubuntu:14.04
MAINTAINER TAGOMORI Satoshi <tagomoris@gmail.com>
LABEL Description="Host image to cross-compile msgpack.gem for mingw32" Vendor="MessagePack Organization" Version="1.0"

USER ubuntu

RUN apt-get install autoconf bison build-essential libssl-dev libyaml-dev libreadline6-dev zlib1g-dev libncurses5-dev libffi-dev libgdbm3 libgdbm-dev
RUN apt-get install gcc-mingw32 mingw-w64
# RUN git?
RUN git clone https://github.com/tagomoris/xbuild.git /home/ubuntu/.xbuild
RUN /home/ubuntu/.xbuild/ruby-install 2.2.2 /home/ubuntu/local/ruby-2.2
RUN PATH=/home/ubuntu/local/ruby-2.2/bin:$PATH /home/ubuntu/local/ruby-2.2/bin/gem install rake-compiler
RUN PATH=/home/ubuntu/local/ruby-2.2/bin:$PATH rake-compiler cross-ruby VERSION=2.2.2 EXTS=--without-extensions
RUN /home/ubuntu/.xbuild/ruby-install 2.1.5 /home/ubuntu/local/ruby-2.1
RUN PATH=/home/ubuntu/local/ruby-2.1/bin:$PATH /home/ubuntu/local/ruby-2.1/bin/gem install rake-compiler
RUN PATH=/home/ubuntu/local/ruby-2.1/bin:$PATH rake-compiler cross-ruby VERSION=2.1.5 EXTS=--without-extensions
RUN /home/ubuntu/.xbuild/ruby-install 2.0.0-p643 /home/ubuntu/local/ruby-2.0
RUN PATH=/home/ubuntu/local/ruby-2.0/bin:$PATH /home/ubuntu/local/ruby-2.0/bin/gem install rake-compiler
RUN PATH=/home/ubuntu/local/ruby-2.0/bin:$PATH rake-compiler cross-ruby VERSION=2.0.0-p643 EXTS=--without-extensions

RUN git clone https://github.com/msgpack/msgpack-ruby.git /home/ubuntu/msgpack-ruby

WORKDIR /home/ubuntu/msgpack-ruby

ENV MSGPACK_REPO https://github.com/msgpack/msgpack-ruby.git
ENV BUILD_BRANCH master
ENV BUILD_POSITION HEAD

ENTRYPOINT ["/home/ubuntu/msgpack-ruby/cross-build.sh", "/home/ubuntu/local/ruby-2.2", "2.0.0-p643:2.1.5:2.2.2"]

