#ifndef INSIGHTS_STACK_LIST_H
#define INSIGHTS_STACK_LIST_H

/// \brief Base class for \ref StackList.
///
template<typename T>
struct StackListEntry
{
    StackListEntry* next{nullptr};
    StackListEntry* prev{nullptr};
};

/// \brief StackList is a container for a list which elements exist only on the stack.
///
/// The purpose is to keep allocation with new and delete away as long as stack seems to be
/// available. The class is range based for loop ready.
/// To use it a class needs to derive from \ref StackListEntry.
///
/// Example:
/// \code
//  class Foo : public StackListEntry<Foo> {...};
/// \endcode
template<typename T>
class StackList
{
public:
    using TStackListEntry = StackListEntry<T>;

    StackList() = default;

    void push(TStackListEntry& entry) noexcept
    {
        entry.next = nullptr;

        if(!mFirst) {
            mFirst       = &entry;
            mFirst->prev = nullptr;

        } else {
            mLast->next = &entry;
            entry.prev  = mLast;
        }

        mLast = &entry;
    }

    T* pop() noexcept
    {
        if(mLast) {
            TStackListEntry* last = mLast;
            mLast                 = mLast->prev;

            if(mLast) {
                mLast->next = nullptr;

                last->next = nullptr;
                last->prev = nullptr;
            } else {
                mFirst = nullptr;
            }

            return static_cast<T*>(last);
        }

        return nullptr;
    }

    T& back() noexcept { return *static_cast<T*>(mLast); }

    constexpr bool empty() const noexcept { return (nullptr == mLast); }

    class StackListIterator
    {
    public:
        StackListIterator(StackList& list)
        : mCurrent{list.mFirst}
        , mLast{list.mLast}
        {
        }

        constexpr StackListIterator& begin() noexcept { return *this; }
        constexpr StackListIterator& end() noexcept { return *this; }

        constexpr T& operator*() noexcept { return *static_cast<T*>(mCurrent); }

        constexpr StackListIterator& operator++() noexcept
        {
            if(mCurrent) {
                mCurrent = mCurrent->next;
            }

            return *this;
        }

        constexpr bool operator!=(const TStackListEntry*) const noexcept { return (nullptr != mCurrent); }

    private:
        TStackListEntry* mCurrent;
        TStackListEntry* mLast;
    };

    constexpr StackListIterator begin() noexcept { return StackListIterator{*this}; }
    constexpr TStackListEntry*  end() noexcept { return mLast; }

private:
    TStackListEntry* mFirst{nullptr};
    TStackListEntry* mLast{nullptr};
};

#endif
