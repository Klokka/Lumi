EXT_RUBY = File.exists?("deps/ruby") ? "deps/ruby" : RbConfig::CONFIG['prefix']
EXT_RUBY_BIN = File.exists?("deps/ruby/bin") ? "deps/ruby/bin" : RbConfig::CONFIG['bindir']
EXT_RUBY_LIB = File.exists?("deps/ruby/lib") ? "deps/ruby/lib" : RbConfig::CONFIG['libdir']
EXT_RUBY_LIBRUBY = File.exists?("deps/ruby/lib/ruby/#{RUBY_V}") ? "deps/ruby/lib/ruby/#{RUBY_V}" : RbConfig::CONFIG['rubylibdir']

# use the platform Ruby claims
require 'rbconfig'

CC = ENV['CC'] ? ENV['CC'] : "gcc"
file_list = ["lumi/*.c"] + %w{lumi/native/gtk.c lumi/http/curl.c}

SRC = FileList[*file_list]
OBJ = SRC.map do |x|
  x.gsub(/\.\w+$/, '.o')
end

ADD_DLL = []

# Linux build environment
CAIRO_CFLAGS = ENV['CAIRO_CFLAGS'] || `pkg-config --cflags cairo`.strip
CAIRO_LIB = ENV['CAIRO_LIB'] ? "-L#{ENV['CAIRO_LIB']}" : `pkg-config --libs cairo`.strip
PANGO_CFLAGS = ENV['PANGO_CFLAGS'] || `pkg-config --cflags pango`.strip
PANGO_LIB = ENV['PANGO_LIB'] ? "-L#{ENV['PANGO_LIB']}" : `pkg-config --libs pango`.strip
png_lib = 'png'

LINUX_CFLAGS = %[-Wall -I#{ENV['LUMI_DEPS_PATH'] || "/usr"}/include #{CAIRO_CFLAGS} #{PANGO_CFLAGS} -I#{RbConfig::CONFIG['archdir']}]
if RbConfig::CONFIG['rubyhdrdir']
  LINUX_CFLAGS << " -I#{RbConfig::CONFIG['rubyhdrdir']} -I#{RbConfig::CONFIG['rubyhdrdir']}/#{LUMI_RUBY_ARCH}"
end

LINUX_LIB_NAMES = %W[#{RUBY_SO} cairo pangocairo-1.0 ungif]

FLAGS.each do |flag|
  LINUX_CFLAGS << " -D#{flag}" if ENV[flag]
end

if ENV['DEBUG']
  LINUX_CFLAGS << " -g -O0 "
else
  LINUX_CFLAGS << " -O "
end
LINUX_CFLAGS << " -DRUBY_1_9"

DLEXT = "so"
LINUX_CFLAGS << " -DLUMI_GTK -fPIC #{`pkg-config --cflags gtk+-2.0`.strip} #{`curl-config --cflags`.strip}"
LINUX_LDFLAGS =" #{`pkg-config --libs gtk+-2.0`.strip} #{`curl-config --libs`.strip} -fPIC -shared"
LINUX_LIB_NAMES << 'jpeg'
LINUX_LIB_NAMES << 'rt'

LINUX_LIBS = LINUX_LIB_NAMES.map { |x| "-l#{x}" }.join(' ')

LINUX_LIBS << " -L#{RbConfig::CONFIG['libdir']} #{CAIRO_LIB} #{PANGO_LIB}"
