#include <iostream>
#define N 5


int main(int argc, char const *argv[])
{

    int height[N] = {13, 2, 5, 1, 10};
    
    int sorted[N] = {0, 1, 2, 3, 4};
    
    // selection sort 
    for (int i = 0; i < N-1; i++) {
        int j = i;
        for (int k = i + 1; k < N; k++) {
        if (height[sorted[k]] > height[sorted[j]])
            j = k;
        }
        int temp = sorted[i];
        sorted[i] = sorted[j];
        sorted[j] = temp;
    }

    for (int i = 0; i < N; i++) {
        std::cout << sorted[i] << " ";
    }

    std::cout << std::endl;
    return 0;
}
