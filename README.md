# A BrlAPI to OSC bridge

A very simple program to make braille displays available via Open Sound Control.

## Building

Create a build directory and point CMake to the location where you cloned this repository:

```console
$ mkdir $(mktemp -d)
$ cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=~/.local/ /path/to/brlosc
$ make install
```

## Usage

By default, brlosc sends messages to port 57120 and listens on udp port 27500.
To change the destination port, pass as first command-line argument.
To change the source port, pass as second argument:

```console
$ BrlOSC 57120 27500
```

### With SuperCollider

By cloning the repository inside of your SuperCollider Extensions
directory, the provided class should be available automatically:

```console
$ cd ~/.local/share/SuperCollider/Extensions
$ git clone https://github.com/mlang/brlosc
```

In SuperCollider:

```sclang
b=BrlOSC({|type, cmd, arg| [type, cmd, arg].postln}, {|err| err.postln});
b.enter;
b.tty;
b.write("Hello from SupoerCollider");
// Pressing braille display keys will print them in the post buffer
b.leave;
b.tty;
b.free;
```

