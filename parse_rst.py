#!/usr/bin/env python3

import sys
import os
import getopt
import docutils
from docutils.frontend import OptionParser
from docutils.utils import new_document
from docutils.parsers.rst import Parser
from docutils.parsers import rst
from docutils.parsers.rst import directives
# Import Docutils document tree nodes module.
from docutils import nodes
# Import Directive base class.
from docutils.parsers.rst import Directive
from docutils.nodes import TextElement, Inline
from docutils.parsers.rst.directives import unchanged

class PostNode(Inline, TextElement):
    pass

class PostDirective(Directive):
    required_arguments = 1
    optional_arguments = 2
    final_argument_whitespace = True
    option_spec = { 'tags' : unchanged, 'category' : unchanged}
    has_content = True

    node_class = None
    """Subclasses must set this to the appropriate admonition node class."""

    def run(self):  
        print(self.arguments)
        print(self.options)
        thenode = PostNode(text=self.arguments[0])
        
        return [thenode]
        
directives.register_directive("post", PostDirective)

inputFile = sys.stdin
outputFile = sys.stdout

settings = OptionParser(components=(Parser,)).get_default_values()
parser = Parser()
input = inputFile.read()
document = new_document(inputFile.name, settings)
parser.parse(input, document)

def prnode(node, level):
    if (node.tagname == 'system_message'):
        return
        
    print(level, ('  '* level), '-', node.tagname, '-------------------------')
    if (node.tagname != '#text'):
        print(level, ('  '* level), node.attlist());
        if (node.tagname == 'PostNode'):
            print(level, ('  '* level), node.options);
    else:
        print(level, ('  '* level), node.astext())
    for c in node.children:
        prnode(c, level + 1)

prnode(document, 0)

output = document.asdom().toxml()
print('==========================================================');
outputFile.write(output)
