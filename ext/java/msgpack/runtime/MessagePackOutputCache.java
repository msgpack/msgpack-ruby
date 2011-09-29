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
import java.util.HashMap;
import java.util.IdentityHashMap;
import java.util.Map;

import org.jruby.RubySymbol;
import org.jruby.runtime.builtin.IRubyObject;


public class MessagePackOutputCache {

    private final Map<IRubyObject, Integer> linkCache = new IdentityHashMap<IRubyObject, Integer>();

    private final Map<String, Integer> symbolCache = new HashMap<String, Integer>();

    public boolean isRegistered(IRubyObject value) {
        if (value instanceof RubySymbol) {
            return isSymbolRegistered(((RubySymbol) value).asJavaString());
        }
        return linkCache.containsKey(value);
    }

    public boolean isSymbolRegistered(String sym) {
        return symbolCache.containsKey(sym);
    }

    public void register(IRubyObject value) {
        if (value instanceof RubySymbol) {
            registerSymbol(value.asJavaString());
        } else {
            linkCache.put(value, Integer.valueOf(linkCache.size()));
        }
    }

    public void registerSymbol(String sym) {
        symbolCache.put(sym, symbolCache.size());
    }

    public void writeLink(MessagePackOutputStream output, IRubyObject value) throws IOException {
        if (value instanceof RubySymbol) {
            writeSymbolLink(output, ((RubySymbol)value).asJavaString());
        } else {
            output.write('@');
            output.writeInt(registeredIndex(value));
        }
    }

    public void writeSymbolLink(MessagePackOutputStream output, String sym) throws IOException {
        output.write(';');
        output.writeInt(registeredSymbolIndex(sym));
    }

    private int registeredIndex(IRubyObject value) {
        if (value instanceof RubySymbol) {
            return registeredSymbolIndex(value.asJavaString());
        } else {
            return linkCache.get(value).intValue();
        }
    }

    private int registeredSymbolIndex(String sym) {
        return symbolCache.get(sym);
    }
}
