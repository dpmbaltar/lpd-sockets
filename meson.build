project('lpd-sockets', 'c', version: '0.1.0')

# Programas opcionales
ruby = find_program('ruby', version: '>= 3.0.0', required: false)
doxygen = find_program('doxygen', version: '>= 1.9.0', required: false)

# Librerías requeridas
gio = dependency('gio-2.0', version: '>= 2.50')
glib = dependency('glib-2.0', version: '>= 2.0')
json_glib =dependency('json-glib-1.0', version: '>= 1.6.0')

subdir('src')
