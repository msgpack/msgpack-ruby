//
// MessagePack for Ruby
//
// Copyright (C) 2008-2011 FURUHASHI Sadayuki
//
//    Licensed under the Apache License, Version 2.0 (the "License");
//    you may not use this file except in compliance with the License.
//    You may obtain a copy of the License at
//
//        http://www.apache.org/licenses/LICENSE-2.0
//
//    Unless required by applicable law or agreed to in writing, software
//    distributed under the License is distributed on an "AS IS" BASIS,
//    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//    See the License for the specific language governing permissions and
//    limitations under the License.
//
package msgpack.runtime;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;


import org.jruby.Ruby;
import org.jruby.RubySymbol;
import org.jruby.runtime.builtin.IRubyObject;


public class MessagePackInputCache {
    private final Ruby runtime;

    private final List<IRubyObject> links = new ArrayList<IRubyObject>();

    private final List<RubySymbol> symbols = new ArrayList<RubySymbol>();

    public MessagePackInputCache(Ruby runtime) {
        this.runtime = runtime;
    }

    public void register(IRubyObject value) {
        selectCache(value).add(value);
    }

    private List selectCache(IRubyObject value) {
        return (value instanceof RubySymbol) ? symbols : links;
    }

    public boolean isLinkType(int c) {
        return c == ';' || c == '@';
    }

    public IRubyObject readLink(MessagePackInputStream input, int type) throws IOException {
        int i = input.unmarshalInt();
        if (type == '@') {
            return linkedByIndex(i);
        }

        assert type == ';';
        return symbolByIndex(i);
    }

    private IRubyObject linkedByIndex(int index) {
        try {
            return links.get(index);
        } catch (IndexOutOfBoundsException e) {
            throw runtime.newArgumentError("dump format error (unlinked, index: " + index + ")");
        }
    }

    private RubySymbol symbolByIndex(int index) {
        try {
            return symbols.get(index);
        } catch (IndexOutOfBoundsException e) {
            throw runtime.newTypeError("bad symbol");
        }
    }
}
