Alice
=====

Efficient Dota 2 replay / demo parser written in C++.

Alice comes in two flavours, `core` and `addon`:

 - `core` includes full access to all replay data
 - `addon` includes additional parsers and structures to work with the data, library and Dota 2 related assets

Both flavours have the same dependencies:

 - [Protocol Buffers (2.5.0)](http://code.google.com/p/protobuf/)
 - [Snappy Compression (1.0.5+)](http://code.google.com/p/snappy/)
 - [Boost (1.52.0+, Header-Only)](http://www.boost.org/)
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
   events (like last-hit gold, and much more), particle systems, etc.
 - _game events_: lower-level messages like Dota TV control (directed camera commands, for example),
   combat log messages, etc.

You are required to subscribe to the data / events you would like to receive. Alice skips any data that doesn't
have a subscriber for performance reasons.

There are certain parts of the replay which no one has figured out yet. Alice provides full access to them
in the following manner:

 - _stringtables_: the contents can be accessed though there is no information on how to parse it
 - _voice data_: binary packets can be written to disk but no one has figured out how the encoding works

Performance
-----------

Alice is heavily optimized and can parse the majority of replays in less than a second. A standard 45 minute public
replay takes about 500ms.

The following settings are available to further tweak the parsing process (with the recommended settings in brackets):

 - forward_dem: Allows the user to receive base-level messages such as raw packet contents (`false`)
 - forward_net: Allows the user to receive network messages (`true`)
 - forward_net_internal: Forwards internal messages e.g. packet-containers and raw packet contents (`false`)
 - forward_user: Whether to receive user messages, requires `forward_net` (`true`)
 - parse_stringtables: Whether to parse stringtables, required when using entities (`true`)
 - parse_entities: Whether to parse entities, required for hero positioning and items (`true`)
 - track_entities: Whether to send specific information about which fields have been modified in an update (`false`)
 - forward_entities: Forward entities like other messages. Might not be required if you have a good parsing setup (`true`)
 - skip_unsubscribed_entities: Skips all non-forwarded entities, increases performance by 20-30% (`true`)

Longer replays often have more messages than their shorter counterparts in the same skill-bracket.
Games with in different skill-brackets and different game modes have more / less messages depending on factors such as
spectator commentary and interaction based messages.

The performance example includes three sample configurations, the following graph shows the differences based on the amount
of messages parsed. The replays used are captains-mode games from the TI3 Qualifiers. 
The x-axis represents the amount of messages parsed, the y-access the time required in milliseconds.

![Image](https://raw.github.com/AliceStats/Alice/master/doc/performance/graph.png)

Memory Usage
------------

Alice was designed to allocate a fixed amount of memory during its initialization stage in order to prevent slow
reallocations and memory fragmentation. How much memory is allocated mainly depends on the combination of the
following two factors:

 - Dem-Stream Type: Using the `dem_stream_memory` requires an additional allocation which equals the size of the replay
 - CPU-Architecture: Compiling the Alice with 64 bit support requires additional memory due to the increased pointer size

The size of the replay only comes into play when using the `dem_stream_memory`. Other than that, replay size only increases
the amount of memory allocated by roughly 200 kb for each 100 MB. Pre-loading the whole replay (as opposed to progressively
reading it) is faster when parsing many replays concurrently.

    MB
    27.73^                                                                       :
         |                        ::@:::::::::@::@@:@::::@@::::::::::::::##:::::@:
         |       @::::::::::::::::: @: :: ::: @::@ :@::: @ :: ::: ::::: :# :::::@:
         |       @:: :::: : : ::: : @: :: ::: @::@ :@::: @ :: ::: ::::: :# :::::@:
         |      :@:: :::: : : ::: : @: :: ::: @::@ :@::: @ :: ::: ::::: :# :::::@:
         |  :::::@:: :::: : : ::: : @: :: ::: @::@ :@::: @ :: ::: ::::: :# :::::@:
         |  : : :@:: :::: : : ::: : @: :: ::: @::@ :@::: @ :: ::: ::::: :# :::::@:
         |  : : :@:: :::: : : ::: : @: :: ::: @::@ :@::: @ :: ::: ::::: :# :::::@:
         |  : : :@:: :::: : : ::: : @: :: ::: @::@ :@::: @ :: ::: ::::: :# :::::@:
         | :: : :@:: :::: : : ::: : @: :: ::: @::@ :@::: @ :: ::: ::::: :# :::::@:
         | :: : :@:: :::: : : ::: : @: :: ::: @::@ :@::: @ :: ::: ::::: :# :::::@:
         | :: : :@:: :::: : : ::: : @: :: ::: @::@ :@::: @ :: ::: ::::: :# :::::@:
         | :: : :@:: :::: : : ::: : @: :: ::: @::@ :@::: @ :: ::: ::::: :# :::::@:
         | :: : :@:: :::: : : ::: : @: :: ::: @::@ :@::: @ :: ::: ::::: :# :::::@:
         | :: : :@:: :::: : : ::: : @: :: ::: @::@ :@::: @ :: ::: ::::: :# :::::@:
         | :: : :@:: :::: : : ::: : @: :: ::: @::@ :@::: @ :: ::: ::::: :# :::::@:
         | :: : :@:: :::: : : ::: : @: :: ::: @::@ :@::: @ :: ::: ::::: :# :::::@:
         | :: : :@:: :::: : : ::: : @: :: ::: @::@ :@::: @ :: ::: ::::: :# :::::@:
         | :: : :@:: :::: : : ::: : @: :: ::: @::@ :@::: @ :: ::: ::::: :# :::::@:
         | :: : :@:: :::: : : ::: : @: :: ::: @::@ :@::: @ :: ::: ::::: :# :::::@:
       0 +----------------------------------------------------------------------->
         0

The graph above represents the amount of memory allocated during the parse of a 170 MB replay. The amount of memory
allocated in the beginning increases rapidly and does only grow very little later.

Taking into account a stack-size of roughly ~6 MB, the overall memory usage will always be between 30 and 33 MB.

Building Alice on Unix
----------------------

Alice utilizes CMake as its build system. To build Alice on Linux or OS X, issue the following commands in the top-level
directory:

    mkdir build
    cd build
    cmake .. -DCMAKE_INSTALL_PREFIX=</where/to/install/alice/> -DBUILD_ADDON=1 -DBUILD_EXAMPLE=1
    make
    make install

This will generate a bin folder containing the example, a lib folder containing shared and static versions of
the library, as well as an include folder with the nessecary headers required to develop with Alice.

If Alice segfaults it might be usefull to turn on debugging by adding `-DDEBUG=X`, where X is a number between 1 and 5,
to the CMake options. If debugging is enabled, Alice prints a lot of information to cout (> 300 MB). It's recommended
to pipe the output in a file and only investigate the last couple of lines. This does not replace a debugger but
it's helpfull in narrowing down certain kinds of errors.

Building Alice on Windows
-------------------------

See the accompanied build-windows.md for instructions on how to build Alice with MSVC.

Please keep in mind that, though Windows is supported, some performance optimizations are not available with MSVC
and are ignored regardless of their state.


Running Alice in your Browser
-----------------------------

Alice can be compiled using [Emscripten](https://github.com/kripken/emscripten). 

See the accompanied build-emscripten.md for instructions on how to proceed.

Using Alice
-----------

The example folder contains a very basic usage example.

You can use the [Devkit](https://github.com/AliceStats/DevKit) to get a quick overview of entities and their properties.

![Image](https://raw.github.com/AliceStats/DevKit/master/doc/screenshot.png)

Thanks
------

Thanks to [edith](https://github.com/dschleck/edith)!

Join us over in #dota2replay on QuakeNet if you have any questions.
