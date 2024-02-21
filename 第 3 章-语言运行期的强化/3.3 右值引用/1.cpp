#include <iostream>
#include <string>

void increase(int &v)
{
  v++;
}

void foo()
{
  double s = 1;
  increase(s);
}

int main()
{
  foo();
  return 0;
}

// 1.cpp:4:6: note: candidate function not viable: no known conversion from 'double' to 'int &' for 1st argument