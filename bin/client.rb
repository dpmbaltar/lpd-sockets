#!/usr/bin/env ruby

require 'json'
require 'optparse'
require 'ostruct'
require 'socket'

# Opciones de línea de comandos
options = OpenStruct.new(
  :host => 'localhost',
  :port => 24000
)

# Obtener opciones
OptionParser.new do |arg|
  arg.on '-h', '--host HOST', 'Host del servidor (i.e. -h localhost)' do |val|
    options.host = val
  end
  arg.on '-p', '--port PORT', 'Puerto del servidor (i.e. -p 24000)' do |val|
    options.port = val.to_i
  end
end.parse!

# Ejecutar loop hasta que el usuario escriba "salir"
loop do

  # Solicitar datos al usuario
  puts 'Escribir mensaje:'
  message = gets
  break if message.chomp.downcase == 'salir'

  # Manejar datos ingresados para el servidor
  message.match /(?<date>\d{4}-\d{1,2}-\d{1,2})(?:[\s]+(?<sign>\w+))?/ do |m|
    message = { date: m[:date], sign: m[:sign] }.to_json
  end

  # Abrir conexión
  socket = TCPSocket.new options.host, options.port

  # Enviar datos
  socket.send message, 0
  puts 'Mensaje enviado:', message
  puts 'Bytes enviados:'
  message.each_byte do |b|
    print '%02x ' % b
  end
  puts

  # Recibir datos
  response = socket.recv(1024).delete("\000")
  puts 'Mensaje recibido:', response
  puts 'Bytes recibidos:'
  response.each_byte do |b|
    print '%02x ' % b
  end
  puts

  # Transformar a JSON
  begin
    puts 'Datos deserializados:'
    response_json = JSON.parse response
    puts JSON.pretty_generate response_json
  rescue JSON::ParserError => error
    puts '[JSON::ParserError: %s]' % error.message
  end

  # Cerrar conexión
  socket.close
  puts 'Conexión cerrada.'

end
