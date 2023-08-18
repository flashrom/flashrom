# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

import os

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

project = 'flashrom'
# copyright = '2023, The flashrom authors'
author = 'The flashrom authors'

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

master_doc = 'index' # this is needed for old versions

extensions = [
    'sphinx.ext.todo'
]

#templates_path = ['_templates']
exclude_patterns = []

# -- Options for Todo extension ----------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/extensions/todo.html

# If this is True, todo and todolist produce output, else they produce nothing. The default is False.
todo_include_todos = False



# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = 'alabaster'
#html_static_path = ['_static']

html_favicon = 'logo/flashrom_icon_color-32x32.ico'


# -- Options for manual page output --------------------------------------------
man_make_section_directory = True
man_show_urls = True
man_pages = [
    ('classic_cli_manpage', project, '', [], 8),
]
