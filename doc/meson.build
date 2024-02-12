
sphinx = find_program('sphinx-build', native : true, required : get_option('man-pages').enabled() or get_option('documentation').enabled())
sphinx_wrapper = meson.current_source_dir() / 'sphinx-wrapper.sh'

man_pages = [
  'flashrom.8'
]

if sphinx.found()
  if get_option('man-pages').auto() or get_option('man-pages').enabled()
    man_outputs = []
    foreach page : man_pages
      man_outputs += 'man' + page.substring(-1)
    endforeach

    custom_target(
      'man-pages',
      command : [sphinx_wrapper, '@OUTDIR@', ' '.join(man_outputs), sphinx, '-b', 'man', '-q', '-d', '@PRIVATE_DIR@', '-Drelease=' + flashrom_version, '@CURRENT_SOURCE_DIR@', '@OUTDIR@'],
      build_always_stale : true, # sphinx handles rebuilds
      output  : man_outputs,
      install : true,
      install_dir : get_option('mandir'),
    )
  endif

  if get_option('documentation').auto() or get_option('documentation').enabled()
    custom_target(
      'documentation',
      command : [sphinx, '-b', 'html', '-q', '-d', '@PRIVATE_DIR@', '-Drelease=' + flashrom_version,'@CURRENT_SOURCE_DIR@', '@OUTDIR@/html'],
      build_always_stale : true, # sphinx handles rebuilds
      output  : 'html',
      install : true,
      install_dir : get_option('datadir') + '/doc/flashrom'
    )
  endif

endif
