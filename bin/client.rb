#!/usr/bin/env ruby

require 'optparse'
require 'ostruct'
require 'socket'

# Opciones de línea de comandos
options = OpenStruct.new
options.host = 'localhost'
options.port = 24000
options.type = 'SP'

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
    case @cond
    when 0 then 'Despejado'
    when 1 then 'Nublado'
    when 2 then 'Neblina'
    when 3 then 'Lluvia'
    when 4 then 'Chubascos'
    when 5 then 'Nieve'
    else 'Desconocida'
    end
  end
end

# Estructura de datos del horóscopo
AstroInfo = Struct.new :sign, :sign_compat, :date_range, :mood do
  attr_accessor :sign, :sign_compat, :mood
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
  def range_from
    "%02d/%02d" % [@date_range[1], @date_range[0]]
  end
  def range_to
    "%02d/%02d" % [@date_range[3], @date_range[2]]
  end
  def sign_s
    sign_to_s @sign
  end
  def sign_compat_s
    sign_to_s @sign_compat
  end
  def sign_to_s sign
    case sign
    when 0 then 'Aries'
    when 1 then 'Tauro'
    when 2 then 'Gemini'
    when 3 then 'Cancer'
    when 4 then 'Leo'
    when 5 then 'Virgo'
    when 6 then 'Libra'
    when 7 then 'Scorpio'
    when 8 then 'Sagitario'
    when 9 then 'Capricornio'
    when 10 then 'Acuario'
    when 11 then 'Piscis'
    else 'Desconocido'
    end
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
      if not info.range_from.empty? then
        puts 'Datos del horóscopo recibidos:'
        puts '- Signo: %d (%s)' % [info.sign, info.sign_s]
        puts '- Compat.: %d (%s)' % [info.sign_compat, info.sign_compat_s]
        puts '- Rango de fechas: %s - %s' % [info.range_from, info.range_to]
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

  # Manejar fechas para el servidor
  if message.match? /[0-9]{4}-[0-9]{2}-[0-9]{2}/ then
    parts = message.split('-').map { |s| s.to_i }
    message = [
      parts[0,1].pack('S'),
      parts[1,1].pack('C'),
      parts[2,1].pack('C')
    ].join
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
