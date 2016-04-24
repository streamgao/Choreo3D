//  Created by Stream Gao on 8/30/15.

#include "Joint.h"

class Bone{
public:
    float len;
    float k=300;
    Joint *a,*b;
    
    Bone();
    Bone(Joint *_a, Joint *_b, float l);
    Bone(Joint *_a, Joint *_b, float l, int ref1, int ref2);
    
    
    void display();
    void update();
    void update(float l);
    float lengthconstrain(float dis);
    glm:: vec3 forceconstrain( glm::vec3 force );
    
    int refnum1, refnum2;
};



