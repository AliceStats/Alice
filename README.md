Alice
=====

Efficient Dota 2 replay / demo parser written in C++.

Alice comes in two flavours, `core` and `addon`:

 - `core` includes full access to all replay data
 - `addon` includes additional parsers and structures to work with the data, library and Dota 2 related assets

Both flavours have the same dependencies:

 - [Protocol Buffers (2.5.0)](http://code.google.com/p/protobuf/)
 - [Snappy Compression (1.0.5+)](http://code.google.com/p/snappy/)
 - [Boost (1.53.0+, Header-Only)](http://www.boost.org/)
 - [CMake (2.8.0+)](http://www.cmake.org/)

Alice supports the following operating systems / compilers:

 - Linux / OS X
 - -> Clang 3.2+
 - -> GCC 4.7.3+ (Requires [Homebrew](http://brew.sh/) or [MacPorts](https://www.macports.org/) on OS X)
 - Windows
 - -> MSVC 2013 ([Express Version](http://www.visualstudio.com/downloads/download-visual-studio-vs#d-express-windows-desktop) should work)

Additional compilers might be supported but haven't been tested.

License
-------

Alice is licensed under the Apache 2.0 License.

Replay Data
-----------

Alice provides access to the following data:

 - _entities_: in-game things like heroes, players, and creeps
 - _modifiers_: auras and effects on in-game entities
 - _user messages_: many different things, including spectator clicks, global chat messages, overhead
   events (like last-hit gold, and much more), particle systems, etc.*
 - _game events_: lower-level messages like Dota TV control (directed camera commands, for example),
   combat log messages, etc.

You are required to subscribe to the data / events you would like to receive. Alice skips any data that doesn't
have a subscriber for performance reasons.

There are certain parts of the replay which no one has figured out yet. Alice provides full access to them
in the following manner:

 - _stringtables_: the contents can be accessed though there is no information on how to parse it
 - _voice data_: binary packets can be written to disk but no one has figured out how the encoding works

Building Alice
--------------

Alice utilizes CMake as its build system. To build Alice on Linux or OS X issue the following commands in the top-level
directory:

    mkdir build
    cd build
    cmake .. -DCMAKE_INSTALL_PREFIX=</where/to/install/alice/> -DBUILD_ADDON=1 -DBUILD_EXAMPLE=1
    make
    make install

This will generate a bin folder containing the example, a lib folder containing shared and static versions of
the library as well as an include folder with the nessecary headers needed to develop with Alice.

Building Alice on Windows requires you to download [CMake for Windows](http://www.cmake.org/files/v2.8/cmake-2.8.12.2-win32-x86.zip).
Put protobuf, snappy and boost in their corresponding directories under /deps/. You don't have to build boost and
and snappy but you need to build protobuf. If you have done so, let cmake generate the MSVC project files. Opening
those should allow you to build the whole project in MSVC.

Binary releases for Windows might be made available in the future when I find a good place to store them.

Using Alice
-----------

The example folder contains a very basic usage example. More applications / examples will follow.

Thanks
------

Thanks to [edith](https://github.com/dschleck/edith) and [skadistats](https://github.com/skadistats/)!
Come join us over in #dota2replay on QuakeNet.