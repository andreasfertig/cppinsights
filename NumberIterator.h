#ifndef INSIGHTS_NUMBER_ITERATOR_H
#define INSIGHTS_NUMBER_ITERATOR_H

template<typename T>
class NumberIterator
{
public:
    NumberIterator(const T num)
    : mNum{num}
    , mCount{0}
    {
    }

    const T& operator*() const { return mCount; }

    NumberIterator& operator++()
    {
        ++mCount;

        return *this;
    }

    bool operator!=(const NumberIterator&) const { return mCount < mNum; }

    const NumberIterator& begin() const { return *this; }
    const NumberIterator& end() const { return *this; }

private:
    const T mNum;
    T       mCount;
};
//-----------------------------------------------------------------------------

#endif /* INSIGHTS_NUMBER_ITERATOR_H */
