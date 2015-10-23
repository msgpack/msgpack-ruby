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
  private final ExtensionEntry[] extensionsByTypeId;

  public ExtensionRegistry() {
    this(new HashMap<RubyClass, ExtensionEntry>());
  }

  private ExtensionRegistry(Map<RubyClass, ExtensionEntry> extensionsByClass) {
    this.extensionsByClass = new HashMap<RubyClass, ExtensionEntry>(extensionsByClass);
    this.extensionsByAncestor = new HashMap<RubyClass, ExtensionEntry>();
    this.extensionsByTypeId = new ExtensionEntry[256];
    for (ExtensionEntry entry : extensionsByClass.values()) {
      if (entry.hasUnpacker()) {
        extensionsByTypeId[entry.getTypeId() + 128] = entry;
      }
    }
  }

  public ExtensionRegistry dup() {
    return new ExtensionRegistry(extensionsByClass);
  }

  public IRubyObject toInternalPackerRegistry(ThreadContext ctx) {
    RubyHash hash = RubyHash.newHash(ctx.getRuntime());
    for (RubyClass extensionClass : extensionsByClass.keySet()) {
      ExtensionEntry entry = extensionsByClass.get(extensionClass);
      if (entry.hasPacker()) {
        hash.put(extensionClass, entry.toPackerTuple(ctx));
      }
    }
    return hash;
  }

  public IRubyObject toInternalUnpackerRegistry(ThreadContext ctx) {
    RubyHash hash = RubyHash.newHash(ctx.getRuntime());
    for (int typeIdIndex = 0 ; typeIdIndex < 256 ; typeIdIndex++) {
      ExtensionEntry entry = extensionsByTypeId[typeIdIndex];
      if (entry != null && entry.hasUnpacker()) {
        IRubyObject typeId = RubyFixnum.newFixnum(ctx.getRuntime(), typeIdIndex - 128);
        hash.put(typeId, entry.toUnpackerTuple(ctx));
      }
    }
    return hash;
  }

  public void put(RubyClass cls, int typeId, IRubyObject packerProc, IRubyObject packerArg, IRubyObject unpackerProc, IRubyObject unpackerArg) {
    ExtensionEntry entry = new ExtensionEntry(cls, typeId, packerProc, packerArg, unpackerProc, unpackerArg);
    extensionsByClass.put(cls, entry);
    extensionsByTypeId[typeId + 128] = entry;
    extensionsByAncestor.clear();
  }

  public IRubyObject lookupUnpackerByTypeId(int typeId) {
    ExtensionEntry e = extensionsByTypeId[typeId + 128];
    if (e != null && e.hasUnpacker()) {
      return e.getUnpackerProc();
    } else {
      return null;
    }
  }

  public IRubyObject[] lookupPackerByClass(RubyClass cls) {
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
    if (e != null && e.hasPacker()) {
      return e.toPackerProcTypeIdPair(cls.getRuntime().getCurrentContext());
    } else {
      return null;
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
    private final IRubyObject packerProc;
    private final IRubyObject packerArg;
    private final IRubyObject unpackerProc;
    private final IRubyObject unpackerArg;

    public ExtensionEntry(RubyClass cls, int typeId, IRubyObject packerProc, IRubyObject packerArg, IRubyObject unpackerProc, IRubyObject unpackerArg) {
      this.cls = cls;
      this.typeId = typeId;
      this.packerProc = packerProc;
      this.packerArg = packerArg;
      this.unpackerProc = unpackerProc;
      this.unpackerArg = unpackerArg;
    }

    public RubyClass getExtensionClass() {
      return cls;
    }

    public int getTypeId() {
      return typeId;
    }

    public boolean hasPacker() {
      return packerProc != null;
    }

    public boolean hasUnpacker() {
      return unpackerProc != null;
    }

    public IRubyObject getPackerProc() {
      return packerProc;
    }

    public IRubyObject getUnpackerProc() {
      return unpackerProc;
    }

    public RubyArray toPackerTuple(ThreadContext ctx) {
      return RubyArray.newArray(ctx.getRuntime(), new IRubyObject[] {RubyFixnum.newFixnum(ctx.getRuntime(), typeId), packerProc, packerArg});
    }

    public RubyArray toUnpackerTuple(ThreadContext ctx) {
      return RubyArray.newArray(ctx.getRuntime(), new IRubyObject[] {cls, unpackerProc, unpackerArg});
    }

    public IRubyObject[] toPackerProcTypeIdPair(ThreadContext ctx) {
      return new IRubyObject[] {packerProc, RubyFixnum.newFixnum(ctx.getRuntime(), typeId)};
    }
  }
}
