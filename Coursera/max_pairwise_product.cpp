#include <iostream>
#include <vector>
#include <algorithm>

long long MaxPairwiseProduct(std::vector<int>& numbers) {
    std::sort(numbers.begin(), numbers.end()); 
    size_t index=numbers.size()-1;
    return static_cast<long long>(numbers[index])*numbers[index-1];
}

int main() {
    int n;
    std::cin >> n;
    std::vector<int> numbers(n);
    for (int i = 0; i < n; ++i) {
        std::cin >> numbers[i];
    }

    std::cout << MaxPairwiseProduct(numbers) << "\n";
    return 0;
}