#! /bin/sh

# A noddy script to automate production of universal binaries on OS X.

make distclean &>/dev/null

export EXTRALDFLAGS="-Wl,-syslibroot,/Developer/SDKs/MacOSX10.3.9.sdk -Wl,-framework,OpenGL"
export CCARGS="-arch ppc -isysroot /Developer/SDKs/MacOSX10.3.9.sdk"
export CC="gcc $CCARGS"
export CXX="g++ $CCARGS"
export SDLOTHERCONFIG="--build=powerpc-apple-darwin7.9.0"
export OTHERCONFIG="--build=powerpc-apple-darwin7.9.0 --disable-sdltest"
./configure --with-internal-libs
make || exit
mv onscripter onscripter.ppc
make distclean &>/dev/null

export EXTRALDFLAGS="-Wl,-syslibroot,/Developer/SDKs/MacOSX10.4u.sdk -Wl,-framework,OpenGL"
export CCARGS="-arch i386 -isysroot /Developer/SDKs/MacOSX10.4u.sdk"
export CC="gcc $CCARGS"
export CXX="g++ $CCARGS"
export SDLOTHERCONFIG="--build=i386-apple-darwin8.11.1"
export OTHERCONFIG="--build=i386-apple-darwin8.11.1 --disable-sdltest"
./configure --with-internal-libs
make || exit
mv onscripter onscripter.intel

lipo -create \
     -arch ppc onscripter.ppc \
     -arch i386 onscripter.intel \
     -output onscripter

cp onscripter onscripter-en