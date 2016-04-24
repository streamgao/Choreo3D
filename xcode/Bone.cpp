//
//  Bone.cpp
//
//  Created by Stream Gao on 8/30/15.
//
//

#include "Bone.h"


using namespace:: ci;

Bone::Bone(){}

Bone::Bone(Joint *_a, Joint *_b, float l){
    a=_a;
    b=_b;
    len = l;
}

Bone::Bone(Joint *_a, Joint *_b, float l, int ref1, int ref2){
    a=_a;
    b=_b;
    len = l;
    refnum1 = ref1;
    refnum2 = ref2;
    std::cout<<"refnum: "<<refnum1<<':'<<refnum2<<std::endl;
}


void Bone::display(){
    gl::color(0.7, 0.7, 1.);
    gl::drawLine(a->location, b->location);
}


void Bone::update(){
    ci::vec3 force = a->location - b->location;
    float stretch = distance(force,glm::vec3(.0,.0,.0) ) - len;
    
    //RESTRICT STRECH,  DENOISE MOCAP DATA
    stretch = std::abs(stretch)>100 ? stretch : 0;
    
    force = normalize(force);
    force *= -1*k*stretch;
    
    force = forceconstrain( force );
    
    //PUSH 2 JOINTS
    a->applyForce(force);
    force *= -1;
    b->applyForce(force);
}


void Bone::update(float l){
    //len = l;
    len = (l+len)/2;
    ci::vec3 force = a->location - b->location;
    float stretch = distance(force,glm::vec3(.0,.0,.0) ) - len;
    
    //RESTRICT STRECH,  DENOISE MOCAP DATA
    stretch = std::abs(stretch)>50 ? stretch : 0;
    
    force = normalize(force);
    force *= -1*k*stretch;
    
    force = forceconstrain( force );
    
    //PUSH 2 JOINTS
    a->applyForce(force);
    force *= -1;
    b->applyForce(force);
}


float Bone::lengthconstrain(float dis){
    if (dis>=2*len) {
        dis=len;
    }
    return dis;
}


glm::vec3 Bone::forceconstrain( glm::vec3 force){
    //if too much, limit the force
//    if( distance(force,glm::vec3(.0,.0,.0) ) > 10000.0 ){
//        force= normalize(force);
//        force *= 10000;
//    }
    //IF TOO FAR, GET BACK
    if ( distance(force,glm::vec3(.0,.0,.0)) >=2*len ) {
        force*=2;
    }
    
    return force;
}








