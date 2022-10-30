#ifndef INSIGHTS_NUMBER_ITERATOR_H
#define INSIGHTS_NUMBER_ITERATOR_H

template<typename T>
class NumberIterator
{
    const T mNum;
    T       mCount{};

public:
    NumberIterator(const T num)
    : mNum{num}
    {
    }

    const T& operator*() const { return mCount; }

    NumberIterator& operator++()
    {
        ++mCount;

        return *this;
    }

    struct sentinel
    {
    };

    bool operator==(sentinel) const { return mCount >= mNum; }

    const NumberIterator& begin() const { return *this; }
    const sentinel        end() const { return {}; }
};
//-----------------------------------------------------------------------------

#endif /* INSIGHTS_NUMBER_ITERATOR_H */
