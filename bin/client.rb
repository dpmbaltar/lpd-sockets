#!/usr/bin/env ruby

require 'optparse'
require 'ostruct'
require 'socket'

# Opciones de línea de comandos
options = OpenStruct.new
options.addr = 'localhost'
options.port = 24000

# Obtener opciones
OptionParser.new do |arg|
  arg.on '-a', '--addr ADDR', 'Dirección (i.e. -a localhost)' do |val|
    options.addr = val
  end
  arg.on '-p', '--port PORT', 'Puerto (i.e. -p 24000)' do |val|
    options.port = val.to_i
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

# Ejecutar loop hasta que el usuario escriba "salir"
begin
  # Abrir conexión
  socket = TCPSocket.new options.addr, options.port

  # Solicitar datos a enviar al usuario
  puts 'Escribir mensaje:'
  message = gets.chomp.strip
  socket.puts message
  puts 'Mensaje enviado.'

  # Mostrar bytes recibidos
  puts 'Bytes recibidos:'
  response = socket.recv WeatherInfo.size
  response.each_byte do |b|
    print '%02x ' % b
  end
  puts

  # Leer bytes en struct WeatherInfo, si hay suficientes datos
  if response.bytesize >= WeatherInfo.size then
    weather_info = WeatherInfo.new.unpack response
    puts 'Datos del clima recibidos:'
    puts '  Fecha: %s' % weather_info.date
    puts '  Condición: %d (%s)' % [weather_info.cond, weather_info.cond_str]
    puts '  Temperatura: %.1f' % weather_info.temp
  end

  # Cerrar conexión
  socket.close
  puts 'Conexión cerrada.'

end until message.downcase == 'salir'
