#include <iostream>
#include <sodium.h>

int main()
{
  if (sodium_init() == -1) {
    std::cout << "no work :(\n";
  }
    std::cout << "it work!\n";
        return 0;

}
 