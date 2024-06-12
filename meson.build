# for ninja:
# meson setup builddir
# cd builddir ; meson compile

# regenerate configuration (run from builddir/)
# meson setup --reconfigure --wipe

project('pcsc-tools', 'c', version : '1.7.1')

# for config.h
conf_data = configuration_data({
  'PACKAGE_VERSION' : '"' + meson.project_version() + '"',
  })

pcsc_dep = dependency('libpcsclite')

executable('pcsc_scan',
  sources : files('pcsc_scan.c'),
  dependencies : pcsc_dep,
  install : true,
  )

# generate config.h
configure_file(output : 'config.h',
  configuration : conf_data)
