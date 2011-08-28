package msgpack.runtime;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;


import org.jruby.Ruby;
import org.jruby.RubySymbol;
import org.jruby.runtime.builtin.IRubyObject;
import org.jruby.runtime.marshal.UnmarshalStream;


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
