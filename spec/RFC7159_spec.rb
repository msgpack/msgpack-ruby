#! /your/favourite/path/to/rspec
# -*- coding: utf-8 -*-

# Copyright (c) 2014 Urabe, Shyouhei.  All rights reserved.
#
# Redistribution  and  use  in  source   and  binary  forms,  with  or  without
# modification, are  permitted provided that the following  conditions are met:
#
#     - Redistributions  of source  code must  retain the  above copyright
#       notice, this list of conditions and the following disclaimer.
#
#     - Redistributions in binary form  must reproduce the above copyright
#       notice, this  list of conditions  and the following  disclaimer in
#       the  documentation  and/or   other  materials  provided  with  the
#       distribution.
#
#     - Neither the name of Internet  Society, IETF or IETF Trust, nor the
#       names of specific contributors, may  be used to endorse or promote
#       products derived from this software without specific prior written
#       permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS”
# AND ANY  EXPRESS OR  IMPLIED WARRANTIES, INCLUDING,  BUT NOT LIMITED  TO, THE
# IMPLIED WARRANTIES  OF MERCHANTABILITY AND  FITNESS FOR A  PARTICULAR PURPOSE
# ARE  DISCLAIMED. IN NO  EVENT SHALL  THE COPYRIGHT  OWNER OR  CONTRIBUTORS BE
# LIABLE  FOR   ANY  DIRECT,  INDIRECT,  INCIDENTAL,   SPECIAL,  EXEMPLARY,  OR
# CONSEQUENTIAL  DAMAGES  (INCLUDING,  BUT   NOT  LIMITED  TO,  PROCUREMENT  OF
# SUBSTITUTE  GOODS OR SERVICES;  LOSS OF  USE, DATA,  OR PROFITS;  OR BUSINESS
# INTERRUPTION)  HOWEVER CAUSED  AND ON  ANY  THEORY OF  LIABILITY, WHETHER  IN
# CONTRACT,  STRICT  LIABILITY, OR  TORT  (INCLUDING  NEGLIGENCE OR  OTHERWISE)
# ARISING IN ANY  WAY OUT OF THE USE  OF THIS SOFTWARE, EVEN IF  ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
# https://github.com/shyouhei/RFC7159

require_relative 'spec_helper'
require 'pathname'

module RFC7159
  def self.load(fp)
    json = fp.read
    raise unless json.valid_encoding?
    msg = MessagePack.from_json json
    begin
      MessagePack.unpack msg
    rescue MessagePack::StackError
      raise unless fp.path.end_with?('/valid/0007-arrays/0004-nested.json')
    end
  end
end

describe RFC7159 do
  this_dir = Pathname.new __dir__

  describe '.load' do
    [
     Encoding::UTF_8,
    ].each do |enc|
      the_dir = this_dir + 'acceptance/valid'
      the_dir.find do |f|
        case f.extname when '.json'
          it "should accept: #{enc}-encoded #{f}" do
            expect do
              f.open "rb", external_encoding: Encoding::UTF_8, internal_encoding: enc do |fp|
                RFC7159.load fp
              end
            end.to_not raise_exception(Exception)
          end
        end
      end

      the_dir = this_dir + 'acceptance/invalid'
      the_dir.find do |f|
        case f.extname when '.txt'
          it "should reject: #{enc}-encoded #{f}" do
            expect do
              f.open "rb", external_encoding: Encoding::UTF_8, internal_encoding: enc do |fp|
                RFC7159.load fp
              end
            end.to raise_exception(Exception)
          end
        end
      end
    end
  end
end

# 
# Local Variables:
# mode: ruby
# coding: utf-8-unix
# indent-tabs-mode: t
# tab-width: 3
# ruby-indent-level: 3
# fill-column: 79
# default-justification: full
# End:
