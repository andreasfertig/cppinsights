/******************************************************************************
 *
 * C++ Insights, copyright (C) by Andreas Fertig
 * Distributed under an MIT license. See LICENSE for details
 *
 ****************************************************************************/

#ifndef INSIGHTS_STRONG_TYPES
#define INSIGHTS_STRONG_TYPES

/// \brief A more than simple typsafe \c bool
///
/// This macro creates a class enum with two enumerators:
/// - \c No
/// - \c Yes
///
/// Usage:
/// \code
/// STRONG_BOOL(AddNewLine);
/// void Foo(AddNewLine);
/// ...
/// void Bar() {
///   Foo(AddNewLine::Yes);
/// }
/// \endcode
#define STRONG_BOOL(typeName)                                                                                          \
    enum class typeName : bool                                                                                         \
    {                                                                                                                  \
        No  = false,                                                                                                   \
        Yes = true                                                                                                     \
    }

#endif /* INSIGHTS_STRONG_TYPES */
