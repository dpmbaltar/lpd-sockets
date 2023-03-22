project('lpd-sockets', 'c', version: '0.1.0')

add_project_dependencies([
  dependency('gio-2.0', version: '>= 2.50'),
  dependency('glib-2.0', version: '>= 2.0'),
],  language: 'c')

server_sources = [
  'src/server/main.c',
  'src/server/tcpserver.c'
]

executable('server', server_sources)
executable('client', 'src/client/main.c')
