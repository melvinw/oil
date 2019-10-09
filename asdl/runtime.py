"""
runtime.py

- Base classes for generated code
- Nodes for pretty printing
"""
from __future__ import print_function

from typing import List, Tuple, Optional, Any

#
# A Homogeneous Tree for Pretty Printing.
#


class _PrettyBase(object):
  pass


class PrettyNode(_PrettyBase):
  """Homogeneous node for pretty-printing."""

  def __init__(self, node_type=None):
    # type: (Optional[str]) -> None
    self.node_type = node_type or ''  # type: str
    # Gah this signature is complicated.
    # Probably should have _PrettyRepeated?
    self.fields = []  # type: List[Tuple[str, _PrettyBase]]

    # Custom hooks set abbrev = True and use the nodes below.
    self.abbrev = False
    self.left = '('
    self.right = ')'
    # Used by abbreviations
    self.unnamed_fields = []  # type: List[_PrettyBase]

  def __repr__(self):
    # type: () -> str
    return '<PrettyNode %s %s>' % (self.node_type, self.fields)


class PrettyArray(_PrettyBase):
  def __init__(self):
    # type: () -> None
    self.children = []  # type: List[_PrettyBase]

  def __repr__(self):
    # type: () -> str
    return '<PrettyArray %s>' % (self.children)


# Color token types
Color_TypeName = 1
Color_StringConst = 2
Color_OtherConst = 3  # Int and bool.  Green?
Color_UserType = 4  # UserType Id
Color_External = 5  # e.g. for value.Obj


class PrettyLeaf(_PrettyBase):
  """Colored string for pretty-printing."""

  def __init__(self, s, e_color):
    # type: (Optional[str], int) -> None
    if s is None:  # hack for repr of MaybeStrArray, which can have 'None'
      self.s = '_'
      self.e_color = Color_OtherConst
    else:
      assert isinstance(s, str), s
      self.s = s
      self.e_color = e_color

  def __repr__(self):
    # type: () -> str
    return '<PrettyLeaf %s %s>' % (self.s, self.e_color)


class ExternalLeaf(_PrettyBase):
  """Leaf node to print an arbitrary objects."""

  def __init__(self, obj):
    # type: (Any) -> None
    self.obj = obj
    self.e_color = Color_External  # alwasy the same

  def __repr__(self):
    # type: () -> str
    return '<ExternalLeaf %s>' % self.obj
