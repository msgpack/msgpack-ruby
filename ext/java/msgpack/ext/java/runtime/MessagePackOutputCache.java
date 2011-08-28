package msgpack.ext.java.runtime;

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
