//  Created by Stream Gao on 8/30/15.

#include "Joint.h"

class Bone{
public:
    float len;
    float k=300;
    Joint *a,*b;
    
    Bone();
    Bone(Joint *_a, Joint *_b, float l);
    
    
    void display();
    void update();
    float lengthconstrain(float dis);
    glm:: vec3 forceconstrain( glm::vec3 force );
};



