#include(CMakeForceCompiler)
set(CMAKE_SYSTEM_NAME "Linux")
set(CMAKE_CROSSCOMPILING TRUE)
set(EGLFS TRUE)
#set(CMAKE_C_COMPILER_WORKS 1)
#set(CMAKE_CXX_COMPILER_WORKS 1)
set(CMAKE_SYSTEM_VERSION 1)
set(CMAKE_SYSTEM_PROCESSOR "armv7-a")
#set(CMAKE_CXX_FLAGS "-fPIC -frtti -fexceptions")
#set(CMAKE_C_FLAGS "-fPIC -fexceptions" )
set(CMAKE_C_COMPILER /opt/sbox-sdk/toolchains/gcc-linaro-5.4.1-aarch64-linux-gnu/bin/arm-linux-gnueabihf-gcc)
set(CMAKE_CXX_COMPILER /opt/sbox-sdk/toolchains/gcc-linaro-5.4.1-aarch64-linux-gnu/bin/arm-linux-gnueabihf-g++)


#CMAKE_FORCE_C_COMPILER(arm-linux-gnueabihf-gcc GNU)
#CMAKE_FORCE_CXX_COMPILER(arm-linux-gnueabihf-g++ GNU)

#If you have installed cross compiler to somewhere else, please specify that path.
SET(COMPILER_ROOT /opt/sbox-sdk/toolchains/gcc-linaro-5.4.1-aarch64-linux-gnu/bin/) 

set(PATH /mnt/rebel/lib/arm-linux-gnueabihf: /mnt/rebel/usr/lib/arm-linux-gnueabihf )


SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

set(CMAKE_FIND_ROOT_PATH /mnt/rebel)
set(CMAKE_SYSROOT /mnt/rebel)

include_directories(SYSTEM /mnt/rebel/usr/include/arm-linux-gnueabihf )
link_directories(/mnt/rebel/lib/arm-linux-gnueabihf)
link_directories(/mnt/rebel/usr/lib/arm-linux-gnueabihf)

set(CMAKE_LIBRARY_PATH
  /mnt/rebel/usr/lib/
  /mnt/rebel/lib/arm-linux-gnueabihf
  )

set( OGRE_DEPENCIES_DIR   /mnt/rebel/usr/lib;/mnt/rebel/lib;/mnt/rebel/lib/arm-linux-gnueabihf;/mnt/rebel/usr/lib/arm-linux-gnueabihf/)
set( BOOST_LIBRARYDIR /mnt/rebel/usr/lib/arm-linux-gnueabihf )
set( ZZip_LIBRARY_REL /mnt/rebel/usr/lib/arm-linux-gnueabihf/libzzip.so)
set( FREETYPE_LIBRARY_REL /mnt/rebel/usr/lib/arm-linux-gnueabihf/libfreetype.so )
set( ZLIB_LIBRARY_REL /mnt/rebel/usr/lib/arm-linux-gnueabihf/imlib2/loaders/zlib.so )
