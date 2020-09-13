//
// Created by hao on 9/13/20.
//

#include "asset_converter.hpp"
#include <iostream>

int main(int argc, char**argv) {
    if(argc != 2) {
        std::cout<<"Provide <path-to-png-directory> as the first argument"<<std::endl;
        return 0;
    }
    parse(argv[1]);
}

