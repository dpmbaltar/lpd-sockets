#!/usr/bin/env ruby

require 'json'
require 'optparse'
require 'ostruct'
require 'socket'

# Opciones de línea de comandos
options = OpenStruct.new(
  :host => 'localhost',
  :port => 24000,
  :type => 'SP'
)

# Obtener opciones
OptionParser.new do |arg|
  arg.on '-h', '--host HOST', 'Host del servidor (i.e. -h localhost)' do |val|
    options.host = val
  end
  arg.on '-p', '--port PORT', 'Puerto del servidor (i.e. -p 24000)' do |val|
    options.port = val.to_i
  end
  arg.on '-t', '--type TYPE', 'Tipo de servidor (SP, SC o SH)' do |val|
    options.type = val
  end
end.parse!

# Condiciones del clima
WeatherCond = [
  'Despejado',
  'Nublado',
  'Neblina',
  'Lluvia',
  'Chubascos',
  'Nieve'
]

# Estructura de datos del clima
WeatherInfo = Struct.new :date, :temp, :cond do
  attr_accessor :date, :temp, :cond
  def self.size
    16
  end
  def unpack data
    @date = data[0,10].to_s
    @cond = data[11,1].unpack('c').join.to_i
    @temp = data[12,4].unpack('F').join.to_f
    self
  end
  def cond_str
    WeatherCond[@cond]
  end
end

# Signos
AstroSign = [
  'Aries',
  'Tauro',
  'Geminis',
  'Cancer',
  'Leo',
  'Virgo',
  'Libra',
  'Scorpio',
  'Sagitario',
  'Capricornio',
  'Acuario',
  'Piscis'
]

# Estructura de datos del horóscopo
AstroInfo = Struct.new :sign, :sign_compat, :date_range, :mood do
  attr_accessor :sign, :sign_compat, :date_range, :mood
  def self.size
    10
  end
  def unpack data
    @sign = data[0,1].unpack('C').join.to_i
    @sign_compat = data[1,1].unpack('C').join.to_i
    @date_range = data[2,4].unpack('C*')
    mood_len = data[6,4].unpack('L').join.to_i
    @mood = data[10,mood_len]
    self
  end
  def sign_s
    AstroSign[@sign]
  end
  def sign_compat_s
    AstroSign[@sign_compat]
  end
end

# Manejo de respuesta para cada tipo de servidor
module Client
  # Servidor principal
  def Client.main response

  end
  # Servidor del horóscopo
  def Client.horoscope response
    # Leer bytes en struct AstroInfo, si hay suficientes datos
    if response.bytesize >= AstroInfo.size then
      info = AstroInfo.new.unpack response
      if not info.date_range.empty? then
        puts 'Datos del horóscopo recibidos:'
        puts '- Signo: %d (%s)' % [info.sign, info.sign_s]
        puts '- Compat.: %d (%s)' % [info.sign_compat, info.sign_compat_s]
        puts '- Fechas: %d/%d - %d/%d' % info.date_range
        puts '- Estado: %s' % info.mood
      else
        puts 'Datos recibidos no válidos'
      end
    end
  end
  # Servidor del clima
  def Client.weather response
    # Leer bytes en struct WeatherInfo, si hay suficientes datos
    if response.bytesize >= WeatherInfo.size then
      weather_info = WeatherInfo.new.unpack response
      if not weather_info.date.empty? then
        puts 'Datos del clima recibidos:'
        puts '- Fecha: %s' % weather_info.date
        puts '- Condición: %d (%s)' % [weather_info.cond, weather_info.cond_str]
        puts '- Temperatura: %.1f' % weather_info.temp
      else
        puts 'Datos recibidos no válidos'
      end
    end
  end
end

# Ejecutar loop hasta que el usuario escriba "salir"
loop do
  # Solicitar datos a enviar al usuario
  puts 'Escribir mensaje:'
  message = gets
  break if message.chomp.downcase == 'salir'

  # Manejar datos ingresados para el servidor
  message.match /(?<date>(\d{4})-(\d{1,2})-(\d{1,2}))(?:[\s]+([\d]))?/ do |m|
    message = { date: m[:date] }.to_json
  end

  # Abrir conexión
  socket = TCPSocket.new options.host, options.port

  # Enviar datos
  socket.send message, 0
  puts 'Mensaje enviado.'

  # Mostrar bytes enviados
  puts 'Bytes enviados:'
  message.each_byte do |b|
    print '%02x ' % b
  end
  puts

  # Mostrar bytes recibidos
  puts 'Bytes recibidos:'
  response = socket.recv 255
  response.each_byte do |b|
    print '%02x ' % b
  end
  puts

  # Manejar respuesta según el tipo de servidor
  case options.type.upcase
  when 'SP' then Client::main response
  when 'SC' then Client::weather response
  when 'SH' then Client::horoscope response
  else break
  end

  # Cerrar conexión
  socket.close
  puts 'Conexión cerrada.'

end
