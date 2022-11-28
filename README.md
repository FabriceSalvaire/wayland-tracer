# Wayland Tracer

Forked from https://github.com/dboyan/wayland-tracer Boyan Ding, 2014

* upgraded Wayland source files
* replaced Autotools by Meson
* updated this readme

## What is wayland-tracer

**wayland-tracer** is a protocol dumper for **Wayland**. It can be used to trace what's on the wire
between the compositor and client, which can be useful for debugging. **It can dump raw binary
data** or interpret data to readable format if XML protocol definitions are provided.

**WARNING: the readable mode similar to WAYLAND_DEBUG=1 requires up to date XML...**

**Notice:** for this use case, [wayland-debug](https://github.com/wmww/wayland-debug) is a better
alternative that features a CLI for viewing, filtering, and setting breakpoints on Wayland protocol
messages.

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

For more uses (such as server-mode, output redirecting, etc.), see the output of `wayland-tracer -h`
or `man wayland-tracer`.
