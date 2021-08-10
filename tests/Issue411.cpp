namespace ns
{
   template <typename T>
   struct foo
   {
      T value;
   };

   template class foo<int>;        // [1]
}

template struct ns::foo<double>;   // [2]

int main()
{
}
