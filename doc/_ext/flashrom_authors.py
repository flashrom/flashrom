import collections
from docutils import nodes
from pathlib import Path
from typing import TYPE_CHECKING

from sphinx.application import Sphinx
from sphinx.util.docutils import SphinxDirective

if TYPE_CHECKING:
    from sphinx.util.typing import ExtensionMetadata


class FlashromAuthorsDirective(SphinxDirective):
    required_arguments = 1
    has_content = True

    def make_table(self, name_counts):
        body = nodes.tbody()
        for name, count in name_counts:
            body += nodes.row(
                "",
                nodes.entry("", nodes.paragraph(text=name)),
                nodes.entry("", nodes.paragraph(text=str(count))),
            )

        return nodes.table(
            "",
            nodes.tgroup(
                "",
                nodes.colspec(colname="name"),
                nodes.colspec(colname="count"),
                nodes.thead(
                    "",
                    nodes.row(
                        "",
                        nodes.entry("", nodes.paragraph(text="Name")),
                        nodes.entry("", nodes.paragraph(text="Number of changes")),
                    ),
                ),
                body,
                cols=2,
            ),
        )

    def make_placeholder(self, contents):
        return nodes.admonition(
            "",
            nodes.title("", text="List not available"),
            *contents,
        )

    def run(self) -> list[nodes.Node]:
        config = self.config.flashrom_authors_list_files

        source_name = self.arguments[0]
        list_file = (config or {}).get(source_name)
        if list_file is None:
            if config is not None:
                available_names = ','.join(config.keys())
                raise self.error(
                    'Undefined authors list file: "{}" (available names: {})'.format(
                        source_name, available_names
                    )
                )
            container = nodes.Element()
            self.state.nested_parse(self.content, 0, container)
            return [self.make_placeholder(container.children)]
        else:
            authors = self.count_authors_from_file(Path(list_file))
            return [self.make_table(authors.most_common())]

    @classmethod
    def count_authors_from_file(cls, list_file: Path) -> collections.Counter:
        # Correcting names may influence the count, so although git shortlog
        # provides us a count there may be multiple entries in the file that
        # must be combined into a single row.
        counter = collections.Counter()

        with list_file.open("r") as f:
            for line in f:
                count, _, name = line.strip().partition("\t")
                fixed_name = cls._correct_known_bad_name(name)
                if fixed_name is not None:
                  counter[fixed_name] += int(count)

        return counter

    _INCORRECT_NAMES = {
        # Commit 9ff3d4cf759161edf6c163f454f11bc9f5328ce3 is missing a '>' in a
        # Co-Authored-By trailer, but this person is already credited as author
        # of that commit so should not be double-counted.
        "persmule <persmule@hardenedlinux.org": None,
    }

    @classmethod
    def _correct_known_bad_name(cls, name: str):
        """
        Return a corrected form of the provided name if it's known to be
        incorrect and should be modified, or None if the name should be
        ignored.  Otherwise, return the provided name.
        """
        return cls._INCORRECT_NAMES.get(name, name)


def setup(app: Sphinx) -> 'ExtensionMetadata':
    app.add_config_value(
        "flashrom_authors_list_files", default=None, rebuild="html", types=[dict]
    )
    app.add_directive("flashrom-authors", FlashromAuthorsDirective)

    return {
        "version": "1",
    }
