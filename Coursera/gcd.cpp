#include <iostream>

int gcd_naive(int a, int b) {
  int current_gcd = 1;
  for (int d = 2; d <= a && d <= b; d++) {
    if (a % d == 0 && b % d == 0) {
      if (d > current_gcd) {
        current_gcd = d;
      }
    }
  }
  return current_gcd;
}

int gcd_Evclid(int a, int b) {
  while(b!=0) {
      int tmp=b;
      b=a%b;
      a=tmp;
  }
  return a;
}

int main() {
  int a, b;
  std::cin >> a >> b;
  std::cout << gcd_Evclid(a, b) << std::endl;
  return 0;
}