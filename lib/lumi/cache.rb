require 'fileutils'
include FileUtils

# locate ~/.lumi
require 'tmpdir'

lib_dir = nil
homes = []
homes << [ENV['HOME'], File.join( ENV['HOME'], '.lumi' )] if ENV['HOME']
homes << [ENV['APPDATA'], File.join( ENV['APPDATA'], 'Lumi' )] if ENV['APPDATA']
homes.each do |home_top, home_dir|
  next unless home_top
  if File.exists? home_top
    lib_dir = home_dir
    break
  end
end
LIB_DIR = lib_dir || File.join(Dir::tmpdir, "lumi")
LIB_DIR.gsub! /\\/, '/'
SITE_LIB_DIR = File.join(LIB_DIR, '+lib')
GEM_DIR = File.join(LIB_DIR, '+gem')
CACHE_DIR = File.join(LIB_DIR, '+cache')

mkdir_p(CACHE_DIR)
$:.unshift SITE_LIB_DIR
$:.unshift GEM_DIR
ENV['GEM_HOME'] = GEM_DIR

require 'rbconfig'
LUMI_RUBY_ARCH = RbConfig::CONFIG['arch']
config = {
  'ruby_install_name' => "lumi --ruby",
  'RUBY_INSTALL_NAME' => "lumi --ruby",
  'prefix' => "#{DIR}", 
  'bindir' => "#{DIR}", 
  'rubylibdir' => "#{DIR}/ruby/lib",
  'datarootdir' => "#{DIR}/share",
  'dvidir' => "#{DIR}/doc/${PACKAGE}",
  'psdir' => "#{DIR}/doc/${PACKAGE}",
  'htmldir' => "#{DIR}/doc/${PACKAGE}",
  'docdir' => "#{DIR}/doc/${PACKAGE}",
  'archdir' => "#{DIR}/ruby/lib/#{LUMI_RUBY_ARCH}",
  'sitedir' => SITE_LIB_DIR,
  'sitelibdir' => SITE_LIB_DIR,
  'sitearchdir' => "#{SITE_LIB_DIR}/#{LUMI_RUBY_ARCH}",
  'LIBRUBYARG_STATIC' => "",
  'libdir' => "#{DIR}",
  'LDFLAGS' => "-L. -L#{DIR}"
}
RbConfig::CONFIG.merge! config
RbConfig::MAKEFILE_CONFIG.merge! config
GEM_CENTRAL_DIR = File.join(DIR, 'ruby/gems/' + RbConfig::CONFIG['ruby_version'])
Dir[GEM_CENTRAL_DIR + "/gems/*"].each do |gdir|
  $: << "#{gdir}/lib"
end
