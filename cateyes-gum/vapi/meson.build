vapis = [
  'cateyes-gum-@0@.vapi'.format(api_version),
]

if host_os_family == 'linux'
  vapis += ['cateyes-gum-linux-@0@.vapi'.format(api_version)]
endif

if get_option('enable_gumjs')
  vapis += ['cateyes-gumjs-@0@.vapi'.format(api_version)]
endif

install_data(vapis, install_dir: 'share/vala/vapi')
