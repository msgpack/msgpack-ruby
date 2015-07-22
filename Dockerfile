FROM ubuntu:14.04
MAINTAINER TAGOMORI Satoshi <tagomoris@gmail.com>
LABEL Description="Host image to cross-compile msgpack.gem for mingw32" Vendor="MessagePack Organization" Version="1.0"

RUN apt-get update -y && apt-get install -y \
            git \
            curl \
            autoconf \
            bison \
            build-essential \
            libssl-dev \
            libyaml-dev \
            libreadline6-dev \
            zlib1g-dev \
            libncurses5-dev \
            libffi-dev \
            libgdbm3 \
            libgdbm-dev \
            mingw-w64 \
            gcc-mingw-w64-i686 \
            gcc-mingw-w64-x86-64 \
    && rm -rf /var/lib/apt/lists/*

RUN useradd ubuntu -d /home/ubuntu -m -U
RUN chown -R ubuntu:ubuntu /home/ubuntu

USER ubuntu

WORKDIR /home/ubuntu

RUN git clone https://github.com/tagomoris/xbuild.git
RUN git clone https://github.com/msgpack/msgpack-ruby.git

RUN /home/ubuntu/xbuild/ruby-install 2.1.5 /home/ubuntu/local/ruby-2.1

ENV PATH /home/ubuntu/local/ruby-2.1/bin:$PATH
RUN gem install rake-compiler
RUN rake-compiler cross-ruby VERSION=2.1.5 HOST=i686-w64-mingw32 EXTS=--without-extensions
RUN rake-compiler cross-ruby VERSION=2.1.5 HOST=x86_64-w64-mingw32 EXTS=--without-extensions

RUN /home/ubuntu/xbuild/ruby-install 2.2.2 /home/ubuntu/local/ruby-2.2

ENV PATH /home/ubuntu/local/ruby-2.2/bin:$PATH
RUN gem install rake-compiler
RUN rake-compiler cross-ruby VERSION=2.2.2 HOST=i686-w64-mingw32 EXTS=--without-extensions
RUN rake-compiler cross-ruby VERSION=2.2.2 HOST=x86_64-w64-mingw32 EXTS=--without-extensions

WORKDIR /home/ubuntu/msgpack-ruby

ENV MSGPACK_REPO https://github.com/msgpack/msgpack-ruby.git
ENV BUILD_BRANCH master
ENV BUILD_POSITION HEAD

### docker run -v `pwd`/pkg:/home/ubuntu/msgpack-ruby/pkg IMAGENAME
CMD ["bash", "-c", "git remote add dockerbuild $MSGPACK_REPO && git fetch dockerbuild && git checkout $BUILD_BRANCH && git pull dockerbuild $BUILD_BRANCH && git reset --hard $BUILD_POSITION && bundle && rake clean && rake cross native gem RUBY_CC_VERSION=2.1.5:2.2.2"]
