Building Alice on Windows
=========================

Building Alice is very straightforward for people who have used CMake before and know their way around C++,
there are a few things that need to be patched in the 3rd party software though.

First of all, download the following dependencies:

 - [CMake 2.8](http://www.cmake.org/files/v2.8/cmake-2.8.12.2.zip)
 - [Boost 1.55](http://sourceforge.net/projects/boost/files/boost/1.55.0/boost_1_55_0.zip/download)
 - [Snappy 1.1.1](http://snappy.googlecode.com/files/snappy-1.1.1.tar.gz)
 - [Protobuf 2.5.0](https://protobuf.googlecode.com/files/protobuf-2.5.0.zip)
 - [Visual Studio 2013 Express](http://www.visualstudio.com/downloads/download-visual-studio-vs#d-express-windows-desktop)

Put boost, snappy and protobuf in the `deps` directory. We only use a few boost headers so building boost is not
nessecary.

Patching / Building Protobuf
-----------------

 - Open the file vsprojects/protobuf.sln with MSVC
 - It will ask you to convert/upgrade the project, you can ignore any errors that pop-up as long as the project opens
 - In the `libprotobuf` project, open the file `zero_copy_stream_impl_light.cc` under `Sources`
 - Put `#include <algorithm>` into line 35. It should look like this:

        // Author: kenton@google.com (Kenton Varda)
        // Based on original Protocol Buffers design by
        // Sanjay Ghemawat, Jeff Dean, and others.
        
        #include <algorithm> // This is the new line
        
        #include <google/protobuf/io/zero_copy_stream_impl_lite.h>
        #include <google/protobuf/stubs/common.h>
        #include <google/protobuf/stubs/stl_util.h>

 - Switch from `Debug` to `Release` in the menu at the top of the screen
 - Right click `libprotobuf` and choose `Build`
 - Do the same for `protoc`
 - There should now be a folder called `Release` within the `vsprojects` folder that contains the build library, just let it sit there, you don't need to move it

Patching Snappy
---------------

 - You don't need to build Snappy directly, it is build alongside Alice
 - Open the file `snappy.h` with any editor, and edit lines 42 - 48 so they look like this:

        #include <stddef.h>
        #include <string>
        
        #include <BaseTsd.h>     // this is new
        typedef SSIZE_T ssize_t; // this is also new
        
        #include "snappy-stubs-public.h"

 - That's it, Snappy can now be build with Alice.

Generating MSVC files for Alice
-------------------------------

 - Open `CMake` which you downloaded before
 - `Where is the source code`: Set this to the root folder of Alice, the `deps` folder should be in it
 - `Where to build the binaries`: Set this to the same folder as above and appened `build/`
 - Press `Configure` at the bottom
 - Select `Visual Studio 2012` and press okay
 - If it can't find any dependencies it will let you know; check the earlier steps to see if you missed something
 - If you want to build the example, click on `BUILD` and tick `BUILD_EXAMPLE`
 - Last but not least, press `Generate`

Building Alice
--------------

 - After you generated the MSVC project files, open the file `ALL_BUILD` in the `build` directory
 - Select `Release` from the menu on the top
 - Right-click `ALL_BUILD` in the project menu and select `Build`
 - That's it, you should be able to run the example / work with the library now
