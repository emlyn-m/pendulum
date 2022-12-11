// Ball.h

class Ball {
    
    public:
        float* color;
        float angle;
        float angularVel;
        float* position;
    
        char real;
    
        Ball();  // default constructor
        Ball(float*);

        void draw();
        void tick(float, float, float*);
};