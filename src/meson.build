client_sources = [
  'client.c',
  'tcpclient.c',
  'util.c',
]

server_sources = [
  'server.c',
  'tcpserver.c',
  'tcpclient.c',
  'util.c',
]

weather_server_sources = [
  'weatherserver.c',
  'tcpserver.c',
  'util.c',
]

hosroscope_server_sources = [
  'horoscopeserver.c',
  'tcpserver.c',
  'util.c',
]

executable('client', client_sources)
executable('server', server_sources)
executable('weather_server', weather_server_sources)
executable('horoscope_server', hosroscope_server_sources)

run_command(
  find_program(join_paths(buildaux, 'meson', 'copy.rb')),
  'horoscope.txt',
  'horoscope.txt',
  check: true
)
