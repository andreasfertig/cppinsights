struct Data
{
  int i;
  
  Data(int x)
  : i{x}
  {
  }
};

Data& Fun(int x)
{
  static Data mData{x};
  
  return mData;
}
