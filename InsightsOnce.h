#ifndef INSIGHTS_ONCE_H
#define INSIGHTS_ONCE_H

namespace clang::insights {

/// \brief A helper object which returns a boolen value just once and toggles it after the first query.
///
/// This allows to simplify code like this:
/// \code
/// bool first{true};
/// for(... : ...) {
///   if(first) {
///      first = false;
///      ...
///   } else {
///     ...
///  }
/// \endcode
///
/// into this:
/// \code
/// OnceTrue first{};
/// for(... : ...) {
///   if(first) {
///      ...
///   } else {
///     ...
///  }
/// \endcode
template<bool VALUE>
class Once
{
public:
    Once() = default;
    Once(bool value)
    : mValue{value}
    {
    }

    operator bool()
    {
        if(VALUE == mValue) {
            mValue = not VALUE;
            return VALUE;
        }

        return not VALUE;
    }

private:
    bool mValue{VALUE};
};

/// Returns true only once, following checks return false.
using OnceTrue = Once<true>;
/// Returns false only once, following checks return true.
using OnceFalse = Once<false>;

}  // namespace clang::insights
#endif /* INSIGHTS_ONCE_H */
