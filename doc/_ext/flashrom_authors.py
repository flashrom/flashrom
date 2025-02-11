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

    def make_table(self, list_file: Path):
        body = nodes.tbody()
        with list_file.open("r") as f:
            for line in f:
                count, _, name = line.strip().partition("\t")

                body += nodes.row(
                    "",
                    nodes.entry("", nodes.paragraph(text=name)),
                    nodes.entry("", nodes.paragraph(text=count)),
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
            return [self.make_table(Path(list_file))]


def setup(app: Sphinx) -> 'ExtensionMetadata':
    app.add_config_value(
        "flashrom_authors_list_files", default=None, rebuild="html", types=[dict]
    )
    app.add_directive("flashrom-authors", FlashromAuthorsDirective)

    return {
        "version": "1",
    }
