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


## Lambdas with static invoker

### Captureless lambdas

Issue [#467](https://github.com/andreasfertig/cppinsights/issues/467) raised awareness that captureless lambdas are
more complicated. According to [expr.prim.lambda.closure] p7, the closure type has a conversion to a function pointer
function:

> The value returned by this conversion function is the address of a function F that, when invoked, has the same effect as invoking the
> closure type’s function call operator on a default-constructed instance of the closure type. F is a
> constexpr function if the function call operator is a constexpr function and is an immediate function if the function call operator is an immediate function.

The code in #467 demonstrates a way to observe that C++ Insights generates less efficient code. Initially, the body of
the call operator was replicated into the invoke function. However, a captureless lambda with a local `static` variable leads to
different results. The latest version forwards the call from the invoke function to the call operator. This is still
less optimal as the compiler does it. Even with `move` and `forward`, we get copies of non-moveable members in a parameter.
At the same time, the compiler seems to be able to directly forward the invoke call to the call operator.


### Lambda captures initialization

C++ Insights shows a constructor for a lambda when it has captures. The compiler does better. It doesn't need a
constructor, it can direct-initialize the members, and by that, the compiler reduces potential copies as they will happen
with the C++ Insights version.


