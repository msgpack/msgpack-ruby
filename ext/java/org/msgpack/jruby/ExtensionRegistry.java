package org.msgpack.jruby;

import org.jruby.Ruby;
import org.jruby.RubyHash;
import org.jruby.RubyArray;
import org.jruby.RubyClass;
import org.jruby.RubyFixnum;
import org.jruby.runtime.ThreadContext;
import org.jruby.runtime.builtin.IRubyObject;

import java.util.Map;
import java.util.HashMap;

public class ExtensionRegistry {
  private final Map<RubyClass, ExtensionEntry> extensionsByClass;
  private final Map<RubyClass, ExtensionEntry> extensionsByAncestor;

  public ExtensionRegistry() {
    this(new HashMap<RubyClass, ExtensionEntry>());
  }

  private ExtensionRegistry(Map<RubyClass, ExtensionEntry> extensionsByClass) {
    this.extensionsByClass = new HashMap<RubyClass, ExtensionEntry>(extensionsByClass);
    this.extensionsByAncestor = new HashMap<RubyClass, ExtensionEntry>();
  }

  public ExtensionRegistry dup() {
    return new ExtensionRegistry(extensionsByClass);
  }

  public IRubyObject toRubyHash(ThreadContext ctx) {
    RubyHash hash = RubyHash.newHash(ctx.getRuntime());
    for (RubyClass extensionClass : extensionsByClass.keySet()) {
      hash.put(extensionClass, extensionsByClass.get(extensionClass).toTuple(ctx));
    }
    return hash;
  }

  public void put(RubyClass cls, int typeId, IRubyObject proc, IRubyObject arg) {
    extensionsByClass.put(cls, new ExtensionEntry(cls, typeId, proc, arg));
    extensionsByAncestor.clear();
  }

  public IRubyObject[] lookup(RubyClass cls) {
    ExtensionEntry e = extensionsByClass.get(cls);
    if (e == null) {
      e = extensionsByAncestor.get(cls);
    }
    if (e == null) {
      e = findEntryByClassOrAncestor(cls);
      if (e != null) {
        extensionsByAncestor.put(e.getExtensionClass(), e);
      }
    }
    if (e == null) {
      return null;
    } else {
      return e.toProcTypePair(cls.getRuntime().getCurrentContext());
    }
  }

  private ExtensionEntry findEntryByClassOrAncestor(final RubyClass cls) {
    ThreadContext ctx = cls.getRuntime().getCurrentContext();
    for (RubyClass extensionClass : extensionsByClass.keySet()) {
      RubyArray ancestors = (RubyArray) cls.callMethod(ctx, "ancestors");
      if (ancestors.callMethod(ctx, "include?", extensionClass).isTrue()) {
        return extensionsByClass.get(extensionClass);
      }
    }
    return null;
  }

  private static class ExtensionEntry {
    private final RubyClass cls;
    private final int typeId;
    private final IRubyObject proc;
    private final IRubyObject arg;

    public ExtensionEntry(RubyClass cls, int typeId, IRubyObject proc, IRubyObject arg) {
      this.cls = cls;
      this.typeId = typeId;
      this.proc = proc;
      this.arg = arg;
    }

    public RubyClass getExtensionClass() {
      return cls;
    }

    public RubyArray toTuple(ThreadContext ctx) {
      return RubyArray.newArray(ctx.getRuntime(), new IRubyObject[] {RubyFixnum.newFixnum(ctx.getRuntime(), typeId), proc, arg});
    }

    public IRubyObject[] toProcTypePair(ThreadContext ctx) {
      return new IRubyObject[] {proc, RubyFixnum.newFixnum(ctx.getRuntime(), typeId)};
    }
  }
}
