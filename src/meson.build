client_sources = [
  'client.c',
  'tcpclient.c',
]

server_sources = [
  'server.c',
  'tcpserver.c',
  'tcpclient.c'
]

weather_server_sources = [
  'weatherserver.c',
  'tcpserver.c',
]

hosroscope_server_sources = [
  'horoserver.c',
  'tcpserver.c',
]

executable('client', client_sources)
executable('server', server_sources)
executable('weather_server', weather_server_sources)
executable('horoscope_server', hosroscope_server_sources)

run_command(
  find_program(
    join_paths(meson.source_root(), 'buildaux', 'meson', 'copy.rb')
  ),
  'horoscope.txt',
  'horoscope.txt'
)
