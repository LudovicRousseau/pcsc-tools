# for ninja:
# meson setup builddir
# cd builddir ; meson compile

# regenerate configuration (run from builddir/)
# meson setup --reconfigure --wipe

project('pcsc-tools', 'c', version : '1.7.2')

# for config.h
conf_data = configuration_data({
  'PACKAGE_VERSION' : '"' + meson.project_version() + '"',
  'pcsc_dir' : get_option('prefix') / get_option('datadir') / 'pcsc',
  'ATR_PARSER' : '"' + get_option('ATRparser') + '"',
  })

extra_link_args = []
# special for Windows
if host_machine.system() == 'windows'
  cc = meson.get_compiler('c')
  pcsc_dep = cc.find_library('winscard')
  extra_link_args += ['-static', '-pthread']
else
  pcsc_dep = dependency('libpcsclite')
endif
threads_dep = dependency('threads')

executable('pcsc_scan',
  sources : files('pcsc_scan.c'),
  dependencies : [pcsc_dep, threads_dep],
  link_args : extra_link_args,
  install : true,
  )

# ATR_analysis
configure_file(output : 'ATR_analysis',
  input : 'ATR_analysis.in',
  install_dir : get_option('bindir'),
  configuration : conf_data,
  )
configure_file(output : 'ATR_analysis.1p',
  input : 'ATR_analysis.1p.in',
  install_dir : get_option('mandir') / 'man1',
  configuration : conf_data,
  )
install_data('smartcard_list.txt',
  install_dir : conf_data.get('pcsc_dir'),
  )

# scriptor
install_data('scriptor',
  install_dir : get_option('bindir'),
  )

# gscriptor
install_data('gscriptor',
  install_dir : get_option('bindir'),
  )
configure_file(output : 'gscriptor.desktop',
  input : 'gscriptor.desktop.in',
  install_dir : get_option('datadir') / 'applications',
  configuration : conf_data,
  )
install_data('gscriptor.png',
  install_dir : conf_data.get('pcsc_dir'),
  )

# man page
install_man(
  'pcsc_scan.1',
  'scriptor.1',
  'gscriptor.1',
  )

# generate config.h
configure_file(output : 'config.h',
  configuration : conf_data)
