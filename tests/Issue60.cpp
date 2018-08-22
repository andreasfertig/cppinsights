#define INSIGHTS_USE_TEMPLATE

#include <map>

struct Person {
    int age;
    int yob() const {
        return 2018 - age;
    }
};

template<typename Key, typename POD>
Key extractKey(Key POD::* pMember);
template<typename Key, typename POD>
POD extractPOD(Key POD::* pMember);

template<auto pMember>
using Key_t = decltype(extractKey(pMember));

template<auto pMember>
using POD_t = decltype(extractPOD(pMember));

template<auto pmember>
struct Index {
  using Key = Key_t<pmember>;
  using POD = POD_t<pmember>;  

  std::map<Key, int> data;
  static Key extractKey(POD const& pod) {
    return pod.*pmember;
  }
};

Index<&Person::age> myAgeIndex;
