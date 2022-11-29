# Wayland Tracer

Forked from https://github.com/dboyan/wayland-tracer Boyan Ding, 2014

* upgraded Wayland source files
* replaced Autotools by Meson
* updated this readme
* code cleanup (modern C)
* improved output readability (ANSI colours, split messages)

## What is wayland-tracer

**wayland-tracer** is a protocol dumper for **Wayland**. It can be used to trace what's on the wire
between the compositor and client, which can be useful for debugging. **It can dump raw binary
data** or interpret data to readable format if XML protocol definitions are provided.

**Notice:** this tool can act as a man in the middle between the Wayland Compositor and a client.

## Building wayland-tracer

Building wayland-tracer is quite simple, it doesn't have many dependencies. Only expat is need for
parsing protocol.

```
git clone https://github.com/dboyan/wayland-tracer.git
cd wayland-tracer
meson build/ --prefix=PREFIX
ninja -C build/ install
```

and you have wayland-tracer.

## Using wayland-tracer

To use wayland-tracer, you first need to have a running wayland compositor.
The most basic use is:

```
wayland-tracer -- PROGRAM [ARGS ...]
```

which launches the program specified and dumps binary data. If you have XML protocol files and want
wayland-tracer to interpret wire data to readable format, you can use `-d` parameter to specifiy XML
protocol files (at least wayland core protocol is needed):

```
$ wayland-tracer -d wayland.xml [-d more-protocols] \
>     -- PROGRAM [ARGS ...]
```

wayland-tracer will interpret the protocol according to xml definition.

**Warning:** you should specify all the protocols used by the client else it could crash.

This behaviour is related to the file descriptor handling in messages, see this
[issue](https://github.com/dboyan/wayland-tracer/issues/1) for more details.

You can run this command to guess which protocols are used by a client:
```
WAYLAND_DEBUG=1 wayland_client 2>&1 | sed -e 's/.*\] //;s/@.*//;s/ -> //' | sort | uniq
```
It will list all the objects involved in the communication, then it is up to you to found the relevant protocol.

You can obtain the protocol XML files in the source code of
[wayland-debug](https://github.com/wmww/wayland-debug) which a CLI for viewing, filtering, and
setting breakpoints on Wayland protocol messages.

For more uses (such as server-mode, output redirecting, etc.), see the output of `wayland-tracer -h`
or `man wayland-tracer`.
