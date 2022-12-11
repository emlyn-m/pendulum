// Matrix.cpp
#include <iostream>
#include <iomanip>
#include "Matrix.h"

Matrix::Matrix(short n, short m) {
    nRows = n;
    nCols = m;
    
    
    values = new float*[nRows];
    for (int i=0; i<n; i++) {
        values[i] = new float[nCols];  // init matrix
        for (int j=0; j<m; j++) {
            values[i][j] = 0;  // zero out matrix
        }
    }
    
};

float* Matrix::operator[] (short i) {
    return values[i];
}


void Matrix::display(const char* name) {
    
    std::cout << "==BEGIN " << name << "==" << std::endl; 
    
    for (int i=0; i<nRows; i++) {
        for (int j=0; j<nCols; j++) {
            std::cout << std::setw(15) << values[i][j];
        }
        std::cout << std::endl;
    }
    
    std::cout << "==END " << name << "==" << std::endl; 

};