// Matrix.h

class Matrix {
    public:
    float** values;
    
        short nRows;
        short nCols;
    
        Matrix(short, short);
        float* operator[] (short);
    
        void display(const char*);
            

};