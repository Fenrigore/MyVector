#include "MyVector.h"

#include <iostream>
#include <stdexcept>
#include <string>


int main() {
    Vector<int> intector;
    intector.PushBack(1);
    intector.PushBack(2);
    intector.PushBack(3);
    intector.PushBack(4);
    intector.PushBack(5);
    for (int i = 0; i < intector.Size(); ++i) {
        std::cout << *(intector.begin() + i) << ", ";
    }
    std::cout << std::endl;

    auto it = intector.Insert(intector.begin() + 1, 999);

    for (int i = 0; i < intector.Size(); ++i) {
        std::cout << *(intector.begin() + i) << ", " ;
    } 
    std::cout << std::endl;

    std::cout << *it << std::endl;
    /*try {
        Test1();
        Test2();
        Test3();
        Test4();
        Test5();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }*/
}
