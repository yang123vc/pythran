""" Replaces **2 by a call to numpy.square. """

from pythran.passmanager import Transformation
from pythran.analyses.ast_matcher import ASTMatcher, AST_any

import ast


class Square(Transformation):

    """
    Replaces **2 by a call to numpy.square.

    >>> import ast
    >>> from pythran import passmanager, backend
    >>> node = ast.parse('a**2')
    >>> pm = passmanager.PassManager("test")
    >>> _, node = pm.apply(Square, node)
    >>> print pm.dump(backend.Python, node)
    import numpy
    numpy.square(a)
    >>> node = ast.parse('numpy.power(a,2)')
    >>> pm = passmanager.PassManager("test")
    >>> _, node = pm.apply(Square, node)
    >>> print pm.dump(backend.Python, node)
    import numpy
    numpy.square(a)
    """

    POW_PATTERN = ast.BinOp(AST_any(), ast.Pow(), ast.Num(2))
    POWER_PATTERN = ast.Call(ast.Attribute(ast.Name('numpy', ast.Load()),
                                           'power', ast.Load()),
                             [AST_any(), ast.Num(2)], [], None, None)

    def __init__(self):
        Transformation.__init__(self)

    def replace(self, value):
        return ast.Call(ast.Attribute(ast.Name('numpy', ast.Load()),
                                      'square', ast.Load()),
                        [value], [], None, None)

    def visit_Module(self, node):
        self.need_import = False
        self.generic_visit(node)
        if self.need_import:
            importIt = ast.Import(names=[ast.alias(name='numpy', asname=None)])
            node.body.insert(0, importIt)
        return node

    def visit_BinOp(self, node):
        self.generic_visit(node)
        if ASTMatcher(Square.POW_PATTERN).search(node):
            self.update = self.need_import = True
            return self.replace(node.left)
        else:
            return node

    def visit_Call(self, node):
        self.generic_visit(node)
        if ASTMatcher(Square.POWER_PATTERN).search(node):
            self.update = self.need_import = True
            return self.replace(node.args[0])
        else:
            return node
