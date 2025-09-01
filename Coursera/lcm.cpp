#include <iostream>

long long lcm_naive(int a, int b) {
  for (long l = 1; l <= (long long) a * b; ++l)
    if (l % a == 0 && l % b == 0)
      return l;

  return (long long) a * b;
}

//LCM(a,b) × GCD(a,b) = a × b
//LCM(a,b) = a × b/GCD(a,b)

int gcd_Evclid(int a, int b) {
  while(b!=0) {
      int tmp=b;
      b=a%b;
      a=tmp;
  }
  return a;
}

long long lcm(int a, int b) {
	return ((long long) a * b)/gcd_Evclid(a,b);
}

int main() {
  int a, b;
  std::cin >> a >> b;
  std::cout << lcm(a, b) << std::endl;
  return 0;
}
