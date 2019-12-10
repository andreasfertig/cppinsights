const int& getTheData() {
  static int theData;
  return theData;
}

int main()
{
  auto& x = getTheData();
}
