#include <iostream>
#include <functional>
#include <map>

int hoge(int n)
{
    std::cout << n << std::endl;
    return n*2;
    
}

int main()
{
    std::map<int, std::function<int(int)> > funcs;
    funcs[1] = [](int n){ return hoge(n); };

    int n = funcs[1](100);
    std::cout << n << std::endl;

    return 0;
}
