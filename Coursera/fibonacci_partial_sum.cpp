#include <iostream>
#include <vector>
using std::vector;
using namespace std;

long long get_fibonacci_partial_sum_naive(long long from, long long to) {
    long long sum = 0;

    long long current = 0;
    long long next  = 1;

    for (long long i = 0; i <= to; ++i) {
        if (i >= from) {
            sum += current;
        }

        long long new_current = next;
        next = next + current;
        current = new_current;
    }

    return sum % 10;
}



long long get_fibonacci_partial_sum(long long from, long long to) {

   int iFrom=static_cast<int>(from);
   int iTo=static_cast<int>(to);

   vector<int> fibN(iTo+1);   
   fibN[0]=0;
   fibN[1]=1;   
   
   for (int i = 2; i <= iTo; ++i) {
       fibN[i]=(fibN[i-2]+fibN[i-1])%10;
   }
/*
   for (int el:fibN) {
       cout << el << " ";
   } cout << "\n";
*/
    long long sum = 0;

    for (int i = iFrom; i <= iTo; ++i) {
        sum += fibN[i];
    }
    return sum % 10;
}

//Last Digit of the Partial Sum of Fibonacci Numbers
//3 7 => 2 + 3 + 5 + 8 + 13 = 31. % 10 =>1

int main() {
    long long from, to;
    std::cin >> from >> to;
    std::cout << get_fibonacci_partial_sum(from, to) << '\n';
}
