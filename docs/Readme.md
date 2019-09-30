# C++ Insights - docs {#readme_docs}

This folder contains files used to generate the documentation which is available at
[docs.cppinsights.io](https://docs.cppinsights.io). [Doxygen](http://doxygen.org) is used to generate the documentation.

The documentation can be created with `cmake`:

```
cmake --build . --target doc
```

There are some pre- and post-processing steps involved.

### Pre-processing

1. `OptionDocumentationGenerator.cpp` is compiled and executed. It parses `InsightsOptions.def` to generate a list of all
C++ Insights options including the help text and the default. For each option there must be a example C++ file in
`examples`. This file is included in help.
2. `OptionDocumentationGenerator.py` uses the previously generated markdown files and execute the C++ Insights binary
   for each sample source file and includes the transformation result in the help file as well.

### Post-processing

Post-processing is done by `postProcessDoxygen.py`:

1. Add `target="_blank"` for each external link in the generated HTML output.
2. Remove unnecessary HTML files. For example, for each markdown file a File Reference file is created but empty.
3. For the remaining HTML files generate a `sitemap.xml` such that search engines can index [docs.cppinsights.io](https://docs.cppinsights.io) properly.
