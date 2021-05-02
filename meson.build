project('loop', 'c', default_options: ['c_std=gnu99'])

libloop = static_library('loop', [
    'src/loop/loop.c'],
    include_directories: ['include'])

libloop_dep = declare_dependency(
    link_with: [libloop],
    include_directories: ['include'])

executable('echo_server', [
    'example/echo_server.c'],
    dependencies: [libloop_dep])

executable('echo_client', [
    'example/echo_client.c'],
    dependencies: [libloop_dep])