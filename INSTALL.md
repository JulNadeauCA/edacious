## Installing Edacious

#### **Prerequisites**

Edacious requires only the Agar library. You can download it from the
[Libagar Website](http://libagar.org/download.html).

#### **Installation under Unix**

1) Execute the configure script. See `./configure --help' for options.
    $ ./configure

2) Compile the libraries and executables.
    $ make depend all
    # make install

3) Let the developers know of your success/failure. If you happen to run
   into trouble, please set a valid return address so we can respond.
    $ cat config.log | mail -s "compiles fine" compile@hypertriton.com

#### **Installation under SGI IRIX**

Under IRIX, we recommend using gcc3 (or SGI cc) because of gcc 2.95 issues
with varargs. There are .tardist packages for gcc3, SDL and freetype
available from [Nekochan](http://www.nekochan.net/).

#### **Installation under Windows**

Under Windows, Edacious can be compiled under Microsoft Visual Studio,
Cygwin and MSYS. Precompiled packages may be available from the Edacious
download page.

#### **Concurrent building**

It is possible to build Edacious outside of the source directory.
Developers may prefer this method, since it results in a faster build
and cleaner source directory.

1) Create the build directory. If available, a memory filesystem is a
   good location to build from.
    $ mkdir w-edacious
    $ cd w-edacious

2) If configure was already executed from the source directory, make sure
   to clean it up or the build will fail:
    $ (cd $HOME/src/edacious && make cleandir)

3) Run configure from the source directory with --srcdir.
    $ $HOME/src/edacious/configure --srcdir=$HOME/src/edacious [...]

4) Now build the libraries and executables as you normally would.
    $ make depend
    $ make
    # make install

