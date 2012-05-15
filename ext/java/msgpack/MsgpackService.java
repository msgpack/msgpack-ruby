package msgpack;

import java.io.IOException;
        
import org.jruby.Ruby;
import org.jruby.runtime.load.BasicLibraryService;

import msgpack.MessagePackLibrary;


public class MsgpackService implements BasicLibraryService {
  public boolean basicLoad(final Ruby runtime) throws IOException {
    new MessagePackLibrary().load(runtime, false);
    return true;
  }
}

