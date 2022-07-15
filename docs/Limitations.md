# C++ Insights Limitations {#limitations}

While the goal is to produce valid and correct code, in some instances, this is not possible. The two main reasons are:

- because the compiler can do things we users can't.
- because the correct way is hard to implement.

This page documents known limitations where the resulting transformation is inaccurate in terms of the standard.


## Point of instantiation of templates

As [#472](https://github.com/andreasfertig/cppinsights/issues/472) notes correctly, the point of instantiation of
templates is not at the precise location as the standard describes it in [temp.point](https://eel.is/c++draft/temp.point).

The implementation in C++ Insights is that all implicit instantiations are placed directly after the primary template. This
is the easiest way implementation-wise. The standard requires that such an instantiation is placed after the namespace
declaration of the POI. Implementing this in C++ Insights would require knowledge about the line where a template was
instantiated and the end of its enclosing namespace. While technically, this information is present in the form of
`SourceLocation`, the traversal of the AST, and the straightforward text-dumping of the nodes does not allow sorting at
this point.


