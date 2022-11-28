# Meson build from https://gitlab.freedesktop.org/wayland/wayland

####################################################################################################

project(
  'wayland-tracer',
  'c',
  version: '1.1.0',
  license: 'MIT',
  meson_version: '>= 0.56.0',
  default_options: [
    'warning_level=2',
    'buildtype=debugoptimized',
    'c_std=c99',
  ]
)
wayland_version = meson.project_version().split('.')

####################################################################################################

config_h = configuration_data()
config_h.set_quoted('PACKAGE', meson.project_name())
config_h.set_quoted('PACKAGE_VERSION', meson.project_version())

cc_args = []
if host_machine.system() != 'freebsd'
  cc_args += ['-D_POSIX_C_SOURCE=200809L']
endif
add_project_arguments(cc_args, language: 'c')

compiler_flags = [
  '-Wno-unused-parameter',
  '-Wstrict-prototypes',
  '-Wmissing-prototypes',
  '-fvisibility=hidden',
]

cc = meson.get_compiler('c')
add_project_arguments(
  cc.get_supported_arguments(compiler_flags),
  language: 'c'
)

foreach h: [ 'sys/prctl.h', 'sys/procctl.h', 'sys/ucred.h' ]
  config_h.set('HAVE_' + h.underscorify().to_upper(), cc.has_header(h))
endforeach

have_funcs = [
  'accept4',
  'mkostemp',
  'posix_fallocate',
  'prctl',
  'memfd_create',
  'mremap',
  'strndup',
]
foreach f: have_funcs
  config_h.set('HAVE_' + f.underscorify().to_upper(), cc.has_function(f))
endforeach
config_h.set10('HAVE_XUCRED_CR_PID', cc.has_member('struct xucred', 'cr_pid', prefix : '#include <sys/ucred.h>'))
have_broken_msg_cmsg_cloexec = false
if host_machine.system() == 'freebsd'
        have_broken_msg_cmsg_cloexec = not cc.compiles('''
#include <sys/param.h> /* To get __FreeBSD_version. */
#if __FreeBSD_version < 1300502 || \
    (__FreeBSD_version >= 1400000 && __FreeBSD_version < 1400006)
/*
 * FreeBSD had a broken implementation of MSG_CMSG_CLOEXEC between 2015 and
 * 2021. Check if we are compiling against a version that includes the fix
 * (https://cgit.freebsd.org/src/commit/?id=6ceacebdf52211).
 */
#error "Broken MSG_CMSG_CLOEXEC"
#endif
''', name : 'MSG_CMSG_CLOEXEC works correctly')
endif
config_h.set10('HAVE_BROKEN_MSG_CMSG_CLOEXEC', have_broken_msg_cmsg_cloexec)

if host_machine.system() == 'freebsd'
  # When building for FreeBSD, epoll(7) is provided by a userspace
  # wrapper around kqueue(2).
  epoll_dep = dependency('epoll-shim')
else
  # Otherwise, assume that epoll(7) is supported natively.
  epoll_dep = []
endif
ffi_dep = dependency('libffi')

decls = [
  { 'header': 'sys/signalfd.h', 'symbol': 'SFD_CLOEXEC' },
  { 'header': 'sys/timerfd.h', 'symbol': 'TFD_CLOEXEC' },
  { 'header': 'time.h', 'symbol': 'CLOCK_MONOTONIC' },
]

foreach d: decls
  if not cc.has_header_symbol(d['header'], d['symbol'], dependencies: epoll_dep, args: cc_args)
    error('@0@ is needed to compile Wayland libraries'.format(d['symbol']))
  endif
endforeach

rt_dep = []
if not cc.has_function('clock_gettime', prefix: '#include <time.h>')
  rt_dep = cc.find_library('rt')
  if not cc.has_function('clock_gettime', prefix: '#include <time.h>', dependencies: rt_dep, args: cc_args)
    error('clock_gettime not found')
  endif
endif

configure_file(
  output: 'config.h',
  configuration: config_h,
)

####################################################################################################

pkgconfig = import('pkgconfig')

####################################################################################################

# root_inc = include_directories('.')
src_inc = include_directories('src')
wayland_inc = include_directories('src/wayland')

wayland_deps = [ epoll_dep, ffi_dep, rt_dep ]
tracer_deps = [ dependency('expat') ]
tracer_args = [ '-include', 'config.h' ]

wayland_tracer_sources = [
  'src/wayland/connection.c',
  'src/wayland/wayland-os.c',
  'src/wayland/wayland-util.c',
  'src/frontend-analyze.c',
  'src/frontend-bin.c',
  'src/tracer-analyzer.c',
  'src/tracer.c',
]
wayland_tracer_includes = [
  src_inc,
  wayland_inc,
]

wayland_tracer = executable(
  'wayland-tracer',
  wayland_tracer_sources,
  c_args: tracer_args,
  include_directories: wayland_tracer_includes,
  dependencies: [ wayland_deps, tracer_deps ],
  install: true
)
