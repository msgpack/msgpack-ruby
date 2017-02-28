This directory contains various  JSON and quasi-JSON texts.  The files
under "valid"  directory are  believed to be  valid JSON  texts, while
those under "invalid"  are believed to be invalid.   And the author of
this library believe  that there is no instance  of text that exhibits
Russell's paradox -like  behaviour, where one cannot say  if that text
(if any) is valid or invalid.

All  files  under this  directory,  including  this  file itself,  are
written  by  me,  Urabe  Shyouhei <shyouhei@ruby-lang.org>.   You  can
safely copy & paste them to your project to test your own JSON parser.

I repeat the license of this  project below, just in case you separate
this test suite form the library.

----------------------------------------------------------------------

Copyright (c) 2014 Urabe, Shyouhei.  All rights reserved.

Redistribution and  use in  source and binary  forms, with  or without
modification, are permitted provided that the following conditions are
met:

  - Redistributions  of source  code must  retain the  above copyright
    notice, this list of conditions and the following disclaimer.

  - Redistributions in binary form  must reproduce the above copyright
    notice, this  list of conditions  and the following  disclaimer in
    the  documentation  and/or   other  materials  provided  with  the
    distribution.

  - Neither the name of Internet  Society, IETF or IETF Trust, nor the
    names of specific contributors, may  be used to endorse or promote
    products derived from this software without specific prior written
    permission.

THIS SOFTWARE  IS PROVIDED BY  THE COPYRIGHT HOLDERS  AND CONTRIBUTORS
“AS IS”  AND ANY EXPRESS  OR IMPLIED WARRANTIES, INCLUDING,  BUT NOT
LIMITED TO, THE IMPLIED  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE  ARE DISCLAIMED. IN NO EVENT  SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL,  EXEMPLARY,  OR  CONSEQUENTIAL  DAMAGES (INCLUDING,  BUT  NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE  GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS  INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF  LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY,  OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING  IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

----------------------------------------------------------------------

P.S.  For those who copy & paste the test cases to your own parser:

  - In convenience  for your own  parser, most (not all)  files starts
    with '[' and ends with ']'.  This is because that should also pass
    older JSON spec (RFC4627).

  - RFC7159 requests us, with using the strongest word (“MUST”) that
    all the valid examples are to  be accepted.  On the other hand the
    RFC _allows_ you to extend  the grammar to, say, accept JavaScript
    comments  and so  on.  So  failure in  parsing valid  examples are
    fatal, but accepting invalid examples might not.

  - RFC7159 tells  no so much  about semantics.  For instance  it does
    not define object equality.  It does not even define the character
    set  of  parsed JSON  strings  (it  DOES  define the  encoding  of
    generated JSON text themselves though).

