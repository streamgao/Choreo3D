//
//  CCL_Dancer.cpp
//  CCL_cinder
//
//  Created by Gene Han on 8/31/15.
//
//

#include "CCL_Dancer.h"

using namespace ci;
using namespace std;

CCL_Dancer::CCL_Dancer( string jsonFileName, int skip, int framerate){
    this->fileName = jsonFileName;
    this->skip = skip;
    this->frameRate = framerate * 2 ;
    
  //  CCL_MocapData ( jsonFileName, this->jointList);
    this->jointList = ccl::loadMotionCaptureFromJson(getAssetPath(jsonFileName) );
    this->totalFrame = this->jointList[0].jointPositions.size();
    this->jointSize = this->jointList.size();
    

//    cout << "Dancer:" << jsonFileName << ", skip:" << skip << ", frameRate:" << framerate << ", totalFrame:" << totalFrame << endl;

    // SHADER
    solidShader = gl::getStockShader( gl::ShaderDef().color() );
    mGlsl = gl::GlslProg::create( loadAsset( "shader.vert" ), loadAsset( "shader.frag" ) );
    mSphereBatch = gl::Batch::create( geom::Sphere(), solidShader );
    
    // GL STUFF
    gl::VboMeshRef body = gl::VboMesh::create( geom::Sphere().subdivisions( 16 ).radius(2) );
    
    //CREATE A CONTAINER TO STORE THE INITIAL POSITIONS FOR INITIALISING THE JOINTS
    std::vector<glm::vec3> positions;
    
    // CREATE THE SPHERES AT THE INITIAL JOINT LOCATIONS
    for ( int i = 0; i < jointList.size(); ++i ) {
     //   if( jointList[i].valid){
            vec3 jointAt = jointList[i].jointPositions[0];
            float instanceX = jointAt.x;
            float instanceY = jointAt.y;
            float instanceZ = jointAt.z;
            
            positions.push_back( vec3( instanceX, instanceY, instanceZ));
     //   }
    }
    
    // create the VBO which will contain per-instance (rather than per-vertex) data
    mInstanceDataVbo = gl::Vbo::create( GL_ARRAY_BUFFER, positions.size() * sizeof(vec3), positions.data(), GL_DYNAMIC_DRAW );
    
    
    // we need a geom::BufferLayout to describe this data as mapping to the CUSTOM_0 semantic, and the 1 (rather than 0) as the last param indicates per-instance (rather than per-vertex)
    geom::BufferLayout instanceDataLayout;
    
    instanceDataLayout.append( geom::Attrib::CUSTOM_0, 3, 0, 0, 1 /* per instance */ );
    
    //NOW ADD IT TO THE VBO MESH THAT WE INITIAL CREATED FOR THE BODY / SKELETON
    body->appendVbo( instanceDataLayout, mInstanceDataVbo );
    
    //FINALLY, BUILD THE BATCH, AND MAP THE CUSTOM_0 ATTRIBUTE TO THE "vInstancePosition" GLSL VERTEX ATTRIBUTE
    mSphereBatch = gl::Batch::create( body, mGlsl, { { geom::Attrib::CUSTOM_0, "vInstancePosition" } } );

}

vector<vec3> CCL_Dancer::getJointPositionsAtFrame(int frame){
    int posAt = frame % totalFrame;
    posAt = posAt > totalFrame ? totalFrame : posAt;
    
    vector<vec3> jointsPosition;
    
    for( int i = 0 ; i < jointList.size() ; i++){
    //   if( jointList[i].valid)
            jointsPosition.push_back(jointList[i].jointPositions[posAt] );
    }
    return jointsPosition;
};

void CCL_Dancer::updateJointPositionAtFrame(int frameAt){
    int frameRounded = frameAt % totalFrame;
    frameAt = frameAt>totalFrame ? totalFrame : frameAt;
    
    vec3 *positions = (vec3*)mInstanceDataVbo->mapReplace();

    for( int i = 0; i < jointList.size(); ++i ) {
    //    if( jointList[i].valid){
            vec3 jointAt = jointList[i].jointPositions[frameRounded];
            float instanceX = jointAt.x;
            float instanceY = jointAt.y;
            float instanceZ = jointAt.z;
            //float instanceZ = 0;
            //just some nonsense math to move the teapots in a wave
            vec3 newPos(vec3(instanceX,instanceY, instanceZ));
        
            positions[i] = newPos;
    //    }
    }
    mInstanceDataVbo->unmap();
};




