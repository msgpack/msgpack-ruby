

import java.io.IOException;

import msgpack.RubyMessagePack;
import msgpack.RubyMessagePackUnpackError;
import msgpack.RubyMessagePackUnpacker;
import org.jruby.Ruby;
import org.jruby.RubyClass;
import org.jruby.RubyModule;
import org.jruby.runtime.load.BasicLibraryService;


public class MsgpackService implements BasicLibraryService {

    public boolean basicLoad(Ruby runtime) throws IOException {
	// define MessagePack module
	RubyModule msgpackModule = runtime.defineModule("MessagePack");
	msgpackModule.defineAnnotatedMethods(RubyMessagePack.class);

	// define MessagePack::Unpacker class
	RubyClass unpackerClass = msgpackModule.defineClassUnder("Unpacker",
		runtime.getObject(), RubyMessagePackUnpacker.ALLOCATOR);
	unpackerClass.defineAnnotatedMethods(RubyMessagePackUnpacker.class);

	// define MessagePack::UnpackError class
	RubyClass unpackerrorClass = msgpackModule.defineClassUnder("UnpackError",
		runtime.getObject(), RubyMessagePackUnpackError.ALLOCATOR);
	unpackerrorClass.defineAnnotatedMethods(RubyMessagePackUnpackError.class);
	RubyMessagePackUnpackError.ERROR = unpackerrorClass;
        return true;
    }
}
