sphinx = find_program('sphinx-build', native : true, required : get_option('man-pages').enabled() or get_option('documentation').enabled())
sphinx_wrapper = meson.current_source_dir() / 'sphinx-wrapper.sh'
collect_authors = meson.current_source_dir() / 'collect-authors.sh'

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

    with_authors_list = get_option('generate_authors_list')
    git = find_program('git', native : true, required : with_authors_list)
    git_dir = meson.project_source_root() / '.git'

    # TODO: investigate whether this version can be lowered.
    version_for_authors_list = '7.2.0'
    # TODO: use sphinx.version().version_compare() instead, after we increase min required meson version to 0.62
    sphinx_for_authors_list = find_program('sphinx-build',
						native : true,
						required : false,
						version : '>=' + version_for_authors_list)

    # When with_authors_list is requested, unsatisfied requirements are an error.
    if with_authors_list.enabled()
      if not git.found()
        error('generate_authors_list was force-enabled but git is not available')
      endif
      if not fs.is_dir(git_dir)
        error('generate_authors_list was force-enabled but a .git directory was not found in the source tree')
      endif
      if not sphinx_for_authors_list.found()
	error('generate_authors_list was force-enabled but sphinx version is too old, min required is '
		+ version_for_authors_list)
      endif
    endif

    if (
      (with_authors_list.enabled() or with_authors_list.auto())
      and git.found() and fs.is_dir(git_dir)
      and sphinx_for_authors_list.found()
    )
      # If requirements are met and authors list is allowed, generate it.
      authors_lst = custom_target(
        'authors_lst',
        output : 'authors.lst',
        command : [collect_authors, 'authors', '@OUTPUT0@', git_dir],
      )
      reviewers_lst = custom_target(
        'reviewers_lst',
        output : 'reviewers.lst',
        command : [collect_authors, 'reviewers', '@OUTPUT0@', git_dir],
      )
      authors_list_options = [
        '-Dflashrom_authors_list_files.authors=' + authors_lst.full_path(),
        '-Dflashrom_authors_list_files.reviewers=' + reviewers_lst.full_path(),
      ]
      doc_depends = [authors_lst, reviewers_lst]
    else
      # Disabled or prerequisites not met. Continue without the authors list.
      # Checks earlier in this file will raise an error if the feature is enabled
      # but the prerequisites aren't met.
      authors_list_options = []
      doc_depends = []

      if with_authors_list.auto()
        # Explain what wasn't satisfied to help the user understand why
        # the authors list is missing.
        if not git.found()
          message('git not found; will not generate authors list')
        elif not fs.is_dir(git_dir)
          message('.git directory not found in project; will not generate authors list')
        endif
      endif
    endif

    custom_target(
      'documentation',
      command : [
        sphinx, '-b', 'html', '-q', '-d', '@PRIVATE_DIR@',
        '-Drelease=' + flashrom_version,
        '@CURRENT_SOURCE_DIR@', '@OUTDIR@/html'
      ] + authors_list_options,
      depends : doc_depends,
      build_always_stale : true, # sphinx handles rebuilds
      output  : 'html',
      install : true,
      install_dir : get_option('datadir') + '/doc/flashrom'
    )
  endif

endif
