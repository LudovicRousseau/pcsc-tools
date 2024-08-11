# for ninja:
# meson setup builddir
# cd builddir ; meson compile

# regenerate configuration (run from builddir/)
# meson setup --reconfigure --wipe

project('pcsc-tools', 'c', version : '1.7.2')

# for config.h
conf_data = configuration_data({
  'PACKAGE_VERSION' : '"' + meson.project_version() + '"',
  })

# special for Windows
if host_machine.system() == 'windows'
  cc = meson.get_compiler('c')
  pcsc_dep = cc.find_library('winscard')
else
  pcsc_dep = dependency('libpcsclite')
endif
threads_dep = dependency('threads')

executable('pcsc_scan',
  sources : files('pcsc_scan.c'),
  dependencies : [pcsc_dep, threads_dep],
  install : true,
  )

# generate config.h
configure_file(output : 'config.h',
  configuration : conf_data)
