project('lpd-sockets', 'c', version: '0.1.0')

buildaux = join_paths(meson.source_root(), 'buildaux')
ruby = find_program('ruby', version: '>= 3.0.5')
doxygen = find_program('doxygen', version: '>= 1.9.0', required: false)
gio = dependency('gio-2.0', version: '>= 2.50')
glib = dependency('glib-2.0', version: '>= 2.0')

add_project_dependencies(gio, glib,  language: 'c')

subdir('src')
