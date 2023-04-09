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

deps = [gio, glib, json_glib]

executable('client', client_sources, dependencies: deps)
executable('server', server_sources, dependencies: deps)
executable('weather_server', weather_server_sources, dependencies: deps)
executable('horoscope_server', hosroscope_server_sources, dependencies: deps)
