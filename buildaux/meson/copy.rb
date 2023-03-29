#!/usr/bin/env ruby

require 'fileutils'

if ARGV.size >= 2 then
  src_dir = File.join ENV['MESON_SOURCE_ROOT'], ENV['MESON_SUBDIR']
  dst_dir = File.join ENV['MESON_BUILD_ROOT'], ENV['MESON_SUBDIR']
  src_file = File.join src_dir, ARGV[0]
  dst_file = File.join dst_dir, ARGV[1]

  if File.exist? src_file and File.directory? dst_dir then
    FileUtils.copy src_file, dst_file, :verbose => true
  end
end
