//
// MessagePack for Ruby
//
// Copyright (C) 2008-2012 FURUHASHI Sadayuki
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
package org.msgpack.ruby;

private class RubyUnpackerImpl {

    // TODO
    /*
    public static final int OBJECT_TYPE_NIL = 0;
    public static final int OBJECT_TYPE_BOOLEAN = 1;
    public static final int OBJECT_TYPE_INTEGER = 2;
    public static final int OBJECT_TYPE_FLOAT = 3;
    public static final int OBJECT_TYPE_RAW = 4;
    public static final int OBJECT_TYPE_ARRAY = 5;
    public static final int OBJECT_TYPE_MAP = 6;

    public static final int PRIMITIVE_CONTAINER_START = 1;
    public static final int PRIMITIVE_OBJECT_COMPLETE = 0;
    public static final int PRIMITIVE_EOF = -1;
    public static final int PRIMITIVE_INVALID_BYTE = -2;
    public static final int PRIMITIVE_STACK_TOO_DEEP = -3;
    public static final int PRIMITIVE_UNEXPECTED_TYPE = -4;

    private static final int STACK_TYPE_ARRAY = 0;
    private static final int STACK_TYPE_MAP_KEY = 1;
    private static final int STACK_TYPE_MAP_VALUE = 2;

    private static final int HEAD_BYTE_REQUIRED = 0xc6;

    private RubyBufferImpl buffer;

    private int headByte;

    private int[] typeStack;
    private int[] countStack;
    private IRubyObject[] objectStack;
    private IRubyObject[] keyStack;

    private int stackDepth;

    private IRubyObject lastObject;

    private RubyString readingRaw;
    private int readingRawRemaining;

    // TODO io

    private int getStackCapacity() {
        return countStack.length;
    }

    public boolean stackIsEmpty() {
        return stackDepth == 0;
    }

    private void resetHeadByte() {
        headByte = HEAD_BYTE_REQUIRED;
    }

    private int objectComplete(IRubyObject object) {
        lastObject = object;
        resetHeadByte();
        return PRIMITIVE_OBJECT_COMPLETE;
    }

    public int read(int targetStackDepth) {
        while(true) {
            int r = readPrimitive();
            if(r < 0) {
                return r;
            }
            if(r == PRIMITIVE_CONTAINER_START) {
                continue;
            }
            // PRIMITIVE_OBJECT_COMPLETE

            if(stackIsEmpty()) {
                return PRIMITIVE_OBJECT_COMPLETE;
            }

            while(true) {
                int type = typeStack[stackDepth-1];
                switch(type) {
                case STACK_TYPE_ARRAY: {
                        IRubyObject object = objectStack[stackDepth-1];
                        // TODO rb_ary_push(object, lastObject);
                    }
                    break;
                case STACK_TYPE_MAP_KEY: {
                        keyStack[stackDepth-1] = lastObject;
                        typeStack[stackDepth-1] = STACK_TYPE_MAP_VALUE;
                    }
                    break;
                case STACK_TYPE_MAP_VALUE: {
                        IRubyObject object = objectStack[stackDepth-1];
                        IRubyObject key = keyStack[stackDepth-1];
                        // TODO rb_hash_aset(object, key, lastObject);
                        typeStack[stackDepth-1] = STACK_TYPE_MAP_KEY;
                    }
                    break;
                }
                int count = --countStack[stackDepth-1];

                if(count != 0) {
                    break;
                }

                // complete and pop stack
                objectComplete(objectStack[stackDepth-1]);

                int depth = --stackDepth;
                if(depth <= targetStackDepth) {
                    return PRIMITIVE_OBJECT_COMPLETE;
                }
                // continue loop
            }
        }
    }

    public int skip(int target_stack_depth) {
        while(true) {
            int r = readPrimitive();
            if(r < 0) {
                return r;
            }
            if(r == PRIMITIVE_CONTAINER_START) {
                continue;
            }
            // PRIMITIVE_OBJECT_COMPLETE

            if(stackIsEmpty()) {
                return PRIMITIVE_OBJECT_COMPLETE;
            }

            while(true) {
                int count = --countStack[stackDepth-1];

                if(count != 0) {
                    break;
                }

                // complete and pop stack
                objectComplete(Qnil);

                int depth = --stackDepth;
                if(depth <= targetStackDepth) {
                    return PRIMITIVE_OBJECT_COMPLETE;
                }
                // continue loop
            }
        }
    }

    private int readHeadByte() {
        // TODO
    }

    private int getHeadByte() {
        int b = headByte;
        if(b == HEAD_BYTE_REQUIRED) {
            b = readHeadByte();
        }
        return b;
    }

    int stackPush(int type, int count, IRubyObject object) {
        resetHeadByte();

        if(getStackCapacity() - stackDepth <= 0) {
            return PRIMITIVE_STACK_TOO_DEEP;
        }

        countStack[stackDepth] = count;
        typeStack[stackDepth] = type;
        objectStack[stackDepth] = object;
        keyStack[stackDepth] = Qnil;

        stackDepth++;
        return PRIMITIVE_CONTAINER_START;
    }

    private int readPrimitive() {
        if(readingRawRemaining > 0) {
            return readRawBodyCont();
        }

        int b = getHeadByte();
        if(b < 0) {
            return b;
        }

        if ((b & 0x80) == 0) { // Positive Fixnum
            // TODO
            return objectComplete(int2fix((short)b));
        }

        if ((b & 0xe0) == 0xe0) { // Negative Fixnum
            // TODO
            return objectComplete(int2fix(b));
        }

        if ((b & 0xe0) == 0xa0) { // FixRaw
            int count = b & 0x1f;
            if(count == 0) {
                // TODO
            }
            readingRawRemaining = count;
            return readRawBodyBegin();
        }

        if ((b & 0xf0) == 0x90) { // FixArray
            int count = b & 0x0f;
            stackPush(STACK_TYPE_ARRAY, count*2, rb_hash_new());
        }

        if ((b & 0xf0) == 0x80) { // FixMap
            int count = b & 0x0f;
            stackPush(STACK_TYPE_MAP_KEY, count*2, rb_hash_new());
        }

        switch (b & 0xff) {
        case 0xc0: // nil
        case 0xc2: // false
        case 0xc3: // true
        case 0xca: // float
        case 0xcb: // double
        case 0xcc: // unsigned int 8
        case 0xcd: // unsigned int 16
        case 0xce: // unsigned int 32
        case 0xcf: // unsigned int 64
        case 0xd0: // signed int 8
        case 0xd1: // signed int 16
        case 0xd2: // signed int 32
        case 0xd3: // signed int 64
        case 0xda: { // raw 16
                int count = in.getShort() & 0xffff;
                if (count == 0) {
                }
            }
        case 0xdb: { // raw 32
            int count = in.getInt();
            }
        case 0xdc: { // array 16
            int count = in.getShort() & 0xffff;
            }
        case 0xdd: { // array 32
            }
        case 0xde: { // map 16
            }
        case 0xdf: { // map 32
            }
        default:
        }
    }

    private int readRawBodyCont() {
        int length = readingRawRemaining;

        // TODO Qnil
        if(readingRaw == Qnil) {
            readingRaw = // TODO new string
        }

        // TODO
    }

    private readRawBodyBegin() {
        // TODO

        return readRawBodyCont();
    }

    public IRubyObject getLastObject();

    public int peekNextObjectType();

    public int skipNil() {
        int b = getHeadByte();
        if(b < 0) {
            return b;
        }
        if(b == 0xc0) {
            return 1;
        }
        return 0;
    }

    public int readArrayHeader() {  // TODO result size
        //int b = getHeadByte();
        //if(b < 0) {
        //    return b;
        //}

        //if(0x90 < b && b < 0x9f) {
        //    *result_size = b & 0x0f;

        //} else if(b == 0xdc) {
        //    /* array 16 */
        //    READ_CAST_BLOCK_OR_RETURN_EOF(cb, uk, 2);
        //    *result_size = _msgpack_be16(cb->u16);

        //} else if(b == 0xdd) {
        //    /* array 32 */
        //    READ_CAST_BLOCK_OR_RETURN_EOF(cb, uk, 4);
        //    *result_size = _msgpack_be32(cb->u32);

        //} else {
        //    return PRIMITIVE_UNEXPECTED_TYPE;
        //}

        return 0;
    }

    public int readMapHeader();  // TODO result size
    */
}

