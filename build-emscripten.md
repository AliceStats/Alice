Building Alice using Emscripten
===============================

Required dependencies
---------------------

 - [Emscripten](https://github.com/kripken/emscripten)
 - [Boost 1.55](http://sourceforge.net/projects/boost/files/boost/1.55.0/boost_1_55_0.zip/download)
 - [Snappy 1.1.1](http://snappy.googlecode.com/files/snappy-1.1.1.tar.gz)
 - [Protobuf-Emscripten](https://github.com/invokr/protobuf-emscripten)

Put the libraries in their corresponding folder under /deps.

I will not go through the process of setting up Emscripten, if you have any troubles check out their 
excellent wiki or mailling list.

Building Boost
--------------

Boost does not need to be build, just put the sources in the corresponding folder.

Building Snappy
---------------

Run the following command in /deps/snappy-1.1.1:

 - `emconfigure ./configure`
 - `emmake make`

Building Protobuf
-----------------

See the README.md in /deps/protobuf-2.5.0/ on how to build Protobuf.

Building Alice
--------------

Run the following commands:

 - `mkdir build && cd build`
 - `CC=emcc CXX=em++ cmake .. -DBUILD_EMSCRIPTEN=1 -DBUILD_ADDON=1 -DBUILD_EXAMPLE=1`
 - `make`

This will generate the examples and the Alice library as LLVM-Bytecode. 

Running the examples
--------------------

Thought the examples compile, you will get an error about being unable to load the replay when you try to run them.
This happens Emscripten's File I/O abstration library is not automatically embedded in the generated Javascript.

In order to run the examples you will have to edit the html generated to include the Filesystem Api.
There are some examples on the [Emscripten Wiki](https://github.com/kripken/emscripten/wiki/Filesystem-Guide).
