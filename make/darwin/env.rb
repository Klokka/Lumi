EXT_RUBY = File.exists?("deps/ruby") ? "deps/ruby" : RbConfig::CONFIG['prefix']
EXT_RUBY_BIN = File.exists?("deps/ruby/bin") ? "deps/ruby/bin" : RbConfig::CONFIG['bindir']
EXT_RUBY_LIB = File.exists?("deps/ruby/lib") ? "deps/ruby/lib" : RbConfig::CONFIG['libdir']
EXT_RUBY_LIBRUBY = File.exists?("deps/ruby/lib/ruby/#{RUBY_V}") ? "deps/ruby/lib/ruby/#{RUBY_V}" : RbConfig::CONFIG['rubylibdir']

CC = ENV['CC'] || "gcc"
SRC = FileList["lumi/*.c", "lumi/native/cocoa.m", "lumi/http/nsurl.m"]

OBJ = SRC.map do |x|
  x.gsub(/\.\w+$/, '.o')
end

ADD_DLL = []

# Linux build environment
pkg_config = `which pkg-config` != ""
pkgs = `pkg-config --list-all`.split("\n").map {|p| p.split.first} unless not pkg_config
if pkg_config and pkgs.include?("cairo") and pkgs.include?("pango")
  CAIRO_CFLAGS = ENV['CAIRO_CFLAGS'] || `pkg-config --cflags cairo`.strip
  CAIRO_LIB = ENV['CAIRO_LIB'] ? "-L#{ENV['CAIRO_LIB']}" : `pkg-config --libs cairo`.strip
  PANGO_CFLAGS = ENV['PANGO_CFLAGS'] || `pkg-config --cflags pango`.strip
  PANGO_LIB = ENV['PANGO_LIB'] ? "-L#{ENV['PANGO_LIB']}" : `pkg-config --libs pango`.strip
else
  # Hack for when pkg-config is not yet installed
  CAIRO_CFLAGS, CAIRO_LIB, PANGO_CFLAGS, PANGO_LIB = "", "", "", ""
end
png_lib = 'png'

LINUX_CFLAGS = %[-Wall -I#{ENV['LUMI_DEPS_PATH'] || "/usr"}/include #{CAIRO_CFLAGS} #{PANGO_CFLAGS} -I#{RbConfig::CONFIG['archdir']}]
if RbConfig::CONFIG['rubyhdrdir']
  LINUX_CFLAGS << " -I#{RbConfig::CONFIG['rubyhdrdir']} -I#{RbConfig::CONFIG['rubyhdrdir']}/#{LUMI_RUBY_ARCH}"
end

LINUX_LIB_NAMES = %W[#{RUBY_SO} pangocairo-1.0 gif pixman-1 jpeg.8]

FLAGS.each do |flag|
  LINUX_CFLAGS << " -D#{flag}" if ENV[flag]
end

if ENV['DEBUG']
  LINUX_CFLAGS << " -g -O0 "
else
  LINUX_CFLAGS << " -O "
end

LINUX_CFLAGS << " -DRUBY_1_9" unless RUBY_2_0
LINUX_CFLAGS << " -DRUBY_2_0" if RUBY_2_0

DLEXT = "dylib"
LINUX_CFLAGS << " -DLUMI_QUARTZ -Wall -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wredundant-decls -fpascal-strings #{RbConfig::CONFIG["CFLAGS"]} -x objective-c -fobjc-exceptions"
LINUX_LDFLAGS = "-framework Cocoa -framework Carbon -dynamiclib -Wl,-single_module INSTALL_NAME"

OSX_SDK = '/Developer/SDKs/MacOSX10.6.sdk'
ENV['MACOSX_DEPLOYMENT_TARGET'] = '10.6'

case ENV['LUMI_OSX_ARCH']
when "universal"
  OSX_ARCH = "-arch i386 -arch x86_64"
when "i386"
  OSX_ARCH = '-arch i386'
else
  OSX_ARCH = '-arch x86_64'
end

LINUX_CFLAGS << " -isysroot #{OSX_SDK} #{OSX_ARCH}"
LINUX_LDFLAGS << " #{OSX_ARCH}"
 
LINUX_LIBS = LINUX_LIB_NAMES.map { |x| "-l#{x}" }.join(' ')

LINUX_LIBS << " -L#{RbConfig::CONFIG['libdir']} #{CAIRO_LIB} #{PANGO_LIB}"

