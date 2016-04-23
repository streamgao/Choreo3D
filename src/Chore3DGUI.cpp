/*      Choreo3D began as a product of the Choreographic Coding Lab, a project of The Motion Bank,
 *      at the New York workshop in August 2015.  The software uses raw data captured from a Vicon
 *      Motion capture system in JSON format.  The data is parsed and the visualisation can be
 *      controlled using the GUI.  The end goal for the project is to allow the user to manipulate
 *      the data during playback and allow them to create new and dynamic performances and pieces
 *      of choreography.
 *
 *      Coded by Craig Pickard, Gene Han and Stream Gao.
 */

#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

//UTILITY INCLUDES
//#include "json/jsoncpp.h"
#include "cinder/CameraUi.h"
#include "cinder/Json.h"
#include "cinder/Utilities.h"
#include "cinder/Log.h"
#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>
#include "cinder/Utilities.h"
#include "cinder/ImageIO.h"

//MY INCLUDES
#include "CCL_MocapData.h"
#include "Skeleton.h"
#include "RibbonFunctions.h"
#include "cinder/Easing.h"
#include "Trail.h"

#include "CinderImGui.h"
#include "GUI_Skeleton.h"
//#include "cinder/params/Params.h"
#include "CCL_Dancer.h"

using namespace ci;
using namespace ci::app;
using namespace std;

/********** DATA ____ GUI ***********************/
int CURRENT_DATA_SET = 0;
int LOADED_DATA_SET = 0;

bool isDancer1 = true;
int fpsDancer1 = 12;

bool isDancer2 = false;
int fpsDancer2 = 12;

bool isDancer3 = false;
int fpsDancer3 = 12;

bool isDancer4 = false;
int fpsDancer4 = 12;

bool isDancer5 = false;
int fpsDancer5 = 12;

int CinderFrameReate = 60;


class Choreo3DApp : public App {
  public:
	void setup() override;
	void mouseDrag( MouseEvent event ) override;
    void keyDown(KeyEvent event) override;      //FOR SAVING SCREENSHOTS
	void update() override;
	void draw() override;
    
    void setupEnviron( int xSize, int zSize, int spacing );     //METHOD FOR SETTING UP THE 3D ENVIRONMENT
    void renderScene();     //METHOD FOR RENDERING THE GRID
    void setupShader();     //METHOD FOR INITIALIZING THE STANDARD SHADER
    
    void drawRibbons();
    void updateRibbons();
    
    void initData();    //METHOD FOR IMPORTING AND INTIALIZING ALL MOCAP DATA
    
    void initGui();     //CONVENIENCE METHOD FOR INITIALIZING THE GUI PARAMS
    void updateGui();   //UPDATE THE GUI EVERY FRAME
    
    void reset();
    void loadDancers();
    
    //CREATE A VERTEX BATCH FOR THE FLOOR MESH
    gl::VertBatchRef	mGridMesh;
    
    //SETUP THE CAMERA
    CameraPersp			mCamera;
    CameraUi			mCamUi;
    
    //GLOBAL DEFUALT SHADER
    gl::GlslProgRef		mGlsl;
    gl::GlslProgRef     solidShader;
    
    //TEST SPHERES
    gl::BatchRef        mSphereBatch;
    gl::VboRef			mInstanceDataVbo;
    
    //NOTE: NEED TO TURN THIS INTO A CLASS
    struct Ribbon{
        vec3              _target;
        std::vector<vec3> _spine;
        std::vector<vec3> _triangles;
        size_t            _joint_index = 0;
    };
    
    int FRAME_COUNT;
    int TOTAL_FRAMES;
    
    int CURRENT_DATA_SET = 0;
    int LOADED_DATA_SET = 0;
    
    //GLOBAL CONTAINER TO HOLD THE JOINT POSITIONS
    vector<CCL_MocapJoint> jointList;
    vector<CCL_Dancer> dancers;
    
    //CREATE A CONTAINER TO STORE THE POSITION OF EACH JOINT FOR A SINGLE FRAME
    //NOTE: SHOULD BE STORING THIS INTO SOME SORT OF BUFFER
    std::vector<glm::vec3> framePositions;
    
    Skeleton skeleton;
    vector<Ribbon> ribbons;
    Trail handTrail;
    
    int mCurrentFrame = 0;
    
    //SETUP GLOBAL UI VARIABLES
    ImGui::Options opts;
    ImGuiStyle style;
    
    //SETUP SKELETON GUI WINDOW
    GUI_Skeleton skeletongui;
    
    bool paused;
    bool camActive;
    bool uiActive;
    bool ribbonsActive;
    bool trailsActive;
    bool MocapSkeletonActive;
    bool NewSkeletonActive;
    bool markersActive;
    bool showGrid;
    bool multipledancers;
    int sphereRadius;
    int frameRate;
    
    ColorA gridColor;
    
    float background;
    
    ColorAf testBK = ColorAf(0.2f,0.2f,0.2f,1.0f);
    ColorAf dancerColor = ColorAf(0.0F,1.f,0.f,1.0f);
    ColorAf markerColour = ColorAf( 1.0f, 1.0f, 1.0f, 1.0f );
    ColorAf ribbonColor = ColorAf(1.0f,1.0f,1.0f,1.0f);
};

void Choreo3DApp::setup()
{
    opts.window( getWindow() );
    frameRate = 12;
    //SET THE GLOBAL FRAMERATE
    setFrameRate(frameRate);
    
    //SETUP THE 3D ENVIRONMENT
    setupEnviron( 5000, 5000, 100 );
    
    //INITIALIZE THE SHADERS
    setupShader();
    
    //SETUP GL
    gl::enableDepthWrite();
    gl::enableDepthRead();
    gl::enableAlphaBlending();
    
    //INITIALISE THE CAMERA (UI ONLY WORKS IF CAMERA IS DISABLED)
    mCamUi = CameraUi( &mCamera, getWindow() );
    
    initData(); //IMPORT THE JSON DATA AND SORT IT INTO A LIST
    
    //NOTE: THIS NEEDS TO BE CLEANED UP
    FRAME_COUNT = 0;
   // TOTAL_FRAMES = jointList[0].jointPositions.size(); //SHOULD PROBABLY PUT A TRY/CATCH HERE
    TOTAL_FRAMES = 3000;   //data begins missing/going crazy at around 3000
    
    std::cout << "total frames: " << TOTAL_FRAMES << std::endl;
    sphereRadius = 2;
    
    gl::VboMeshRef body = gl::VboMesh::create( geom::Sphere().subdivisions( 16 ).radius(sphereRadius) );
    
    // CREATE THE SPHERES AT THE INITIAL JOINT LOCATIONS
    for ( int i = 0; i < jointList.size(); ++i ) {
        float instanceX = jointList[i].jointPositions[0].x;
        float instanceY = jointList[i].jointPositions[0].y;
        float instanceZ = jointList[i].jointPositions[0].z;
        // float instanceZ = 0;
        
        framePositions.push_back( vec3( instanceX, instanceY, instanceZ));
    }
    
    skeleton = Skeleton(framePositions);
    handTrail = Trail(framePositions[26]);
    
    // create the VBO which will contain per-instance (rather than per-vertex) data
    mInstanceDataVbo = gl::Vbo::create( GL_ARRAY_BUFFER, framePositions.size() * sizeof(vec3), framePositions.data(), GL_DYNAMIC_DRAW );
    
    // we need a geom::BufferLayout to describe this data as mapping to the CUSTOM_0 semantic, and the 1 (rather than 0) as the last param indicates per-instance (rather than per-vertex)
    geom::BufferLayout instanceDataLayout;
    
    instanceDataLayout.append( geom::Attrib::CUSTOM_0, 3, 0, 0, 1  );
    
    //NOW ADD IT TO THE VBO MESH THAT WE INITIAL CREATED FOR THE BODY / SKELETON
    body->appendVbo( instanceDataLayout, mInstanceDataVbo );
    
    //FINALLY, BUILD THE BATCH, AND MAP THE CUSTOM_0 ATTRIBUTE TO THE "vInstancePosition" GLSL VERTEX ATTRIBUTE
    mSphereBatch = gl::Batch::create( body, mGlsl, { { geom::Attrib::CUSTOM_0, "vInstancePosition" } } );
    
    //PRINT OUT JOINT INDEX AND NAME OF JOINT
    
    for (int i = 0; i < jointList.size(); i++){
        std::cout << "index: " << i << ", Joint name: " << jointList[i].jointName << std::endl;
    }
    
    //SETUP RIBBONS
    for (auto i = 0; i < jointList.size(); i += 1){
        auto r = Ribbon();
        auto pos = framePositions[i];
        r._spine.assign(10, pos);
        r._joint_index = i;
        r._target = pos;
        ribbons.push_back(r);
    }
     
    //SETUP THE GUI
    initGui();
}

//-------------------- IMPORT DATA -------------------------
void Choreo3DApp::initData()
{
    jointList = ccl::loadMotionCaptureFromJson(getAssetPath("CCL_JOINT_CCL3_00_skip10.json"));    
    loadDancers();
}

void Choreo3DApp::loadDancers(){
    vector<std::string> dataVector = {
        "CCL_JOINT_CCL4_00_skip4.json",
        "CCL_JOINT_CCL4_01_skip4.json",
        "CCL_JOINT_CCL4_02_skip4.json",
        "CCL_JOINT_CCL4_03_skip8.json",
        "CCL_JOINT_CCL4_04_skip8.json"};
    
    CCL_Dancer dancer1 = CCL_Dancer(dataVector[0], 30, 360);
    CCL_Dancer dancer2 = CCL_Dancer(dataVector[1], 4, 240);
    CCL_Dancer dancer3 = CCL_Dancer(dataVector[2], 4, 240);
    CCL_Dancer dancer4 = CCL_Dancer(dataVector[3], 4, 240);
    CCL_Dancer dancer5 = CCL_Dancer(dataVector[4], 8, 240);
    
    dancers.push_back(dancer1);
    dancers.push_back(dancer2);
    dancers.push_back(dancer3);
    dancers.push_back(dancer4);
    dancers.push_back(dancer5);
}


void Choreo3DApp::mouseDrag( MouseEvent event )
{
}

void Choreo3DApp::update()
{
    setFrameRate(frameRate);
    //DISABLE CAMERA INTERACTION IF MOUSE IS OVER UI REGION
    if (getMousePos().x > 3 * getWindowWidth()/4. && camActive)
    {
        camActive = false;
        mCamUi.disconnect();
        mCamUi.disable();
    } else {
        if (!camActive){
            mCamUi.connect(getWindow());
            mCamUi.enable();
        }
        camActive = true;
    }
    
    mGlsl->uniform("uColor", markerColour );
    
    if (!paused){
        if( isDancer1){
            dancers[0].updateJointPositionAtFrame(FRAME_COUNT/(CinderFrameReate/fpsDancer1));
        }
        if( isDancer2){
            dancers[1].updateJointPositionAtFrame(FRAME_COUNT/(CinderFrameReate/fpsDancer2));
        }
        if( isDancer3){
            dancers[2].updateJointPositionAtFrame(FRAME_COUNT/(CinderFrameReate/fpsDancer3));
        }
        if( isDancer4){
            dancers[3].updateJointPositionAtFrame(FRAME_COUNT/(CinderFrameReate/fpsDancer4));
        }
        if( isDancer5){
            dancers[4].updateJointPositionAtFrame(FRAME_COUNT/(CinderFrameReate/fpsDancer5));
        }
        
        glm::vec3 *newPositions = (glm::vec3*)mInstanceDataVbo->mapReplace();
        for( int i = 0; i < jointList.size(); ++i )
        {
            float instanceX = jointList[i].jointPositions[FRAME_COUNT].x;
            float instanceY = jointList[i].jointPositions[FRAME_COUNT].y;
            float instanceZ = jointList[i].jointPositions[FRAME_COUNT].z;
            
            vec3 newPos(vec3(instanceX,instanceY, instanceZ)); //CREATE A NEW VEC3 FOR UPDATING THE VBO
            framePositions[i] = newPos;
        }
        
        //REPLACE VEC3s IN VBO BY INCREMENTING THE POINTER
        for (int i = 0; i < framePositions.size(); i++){
            *newPositions++ = framePositions[i];
        }
        handTrail.update(framePositions[26], dancerColor);
        skeleton.update(framePositions);

        mInstanceDataVbo->unmap();
         
        if (ribbonsActive) updateRibbons();
        
        //MANUALLY INCREMENT THE FRAME, IF THE FRAME_COUNT EXCEEDS TOTAL FRAMES, RESET THE COUNTER
        if (FRAME_COUNT < TOTAL_FRAMES){
            FRAME_COUNT += 1;
            //std::cout<<"frame count: "<<FRAME_COUNT<<endl;
        } else {//paused
            FRAME_COUNT = 0;
        }
        mCurrentFrame++; //MANUALLY ADVANCE THE CURRENT FRAME - WITH RESPECT TO THE DANCERS
    }
    updateGui();
}

void Choreo3DApp::draw()
{
    gl::clear(ColorAf(testBK));
    //THIS MAY NEED TO BE CLEANED UP
    vector<std::string> dataVector = {"CCL_JOINT_CCL3_00_skip10.json"};
    
    if( CURRENT_DATA_SET != LOADED_DATA_SET){
        jointList = ccl::loadMotionCaptureFromJson(getAssetPath(dataVector[CURRENT_DATA_SET]));
        
        FRAME_COUNT = 0;
        //TOTAL_FRAMES = jointList[0].jointPositions.size(); //SHOULD PROBABLY PUT A TRY/CATCH HERE
        TOTAL_FRAMES = 3000; // data begins missing at around 3000
        
        gl::VboMeshRef body = gl::VboMesh::create( geom::Sphere().subdivisions( 16 ).radius(4) );
        //CREATE A CONTAINER TO STORE THE INITIAL POSITIONS FOR INITIALISING THE JOINTS
        std::vector<glm::vec3> positions;
        
        // CREATE THE SPHERES AT THE INITIAL JOINT LOCATIONS
        for ( int i = 0; i < jointList.size(); ++i ) {
            glm::vec3 jointAt = jointList[i].jointPositions[i];
            positions.push_back( vec3( jointAt.x, jointAt.y, jointAt.z) );
        }
      
        // create the VBO which will contain per-instance (rather than per-vertex) data
        mInstanceDataVbo = gl::Vbo::create( GL_ARRAY_BUFFER, positions.size() * sizeof(vec3), positions.data(), GL_DYNAMIC_DRAW );
        
        // we need a geom::BufferLayout to describe this data as mapping to the CUSTOM_0 semantic, and the 1 (rather than 0) as the last param indicates per-instance (rather than per-vertex)
        geom::BufferLayout instanceDataLayout;
        instanceDataLayout.append( geom::Attrib::CUSTOM_0, 3, 0, 0, 1);
        //NOW ADD IT TO THE VBO MESH THAT WE INITIAL CREATED FOR THE BODY / SKELETON
        body->appendVbo( instanceDataLayout, mInstanceDataVbo );
        
        //FINALLY, BUILD THE BATCH, AND MAP THE CUSTOM_0 ATTRIBUTE TO THE "vInstancePosition" GLSL VERTEX ATTRIBUTE
        mSphereBatch = gl::Batch::create( body, mGlsl, { { geom::Attrib::CUSTOM_0, "vInstancePosition" } } );
        LOADED_DATA_SET = CURRENT_DATA_SET;
    }
    
    if(isDancer1){
        dancers[0].mSphereBatch->drawInstanced(dancers[0].jointSize);
    }
    if(isDancer2){
        dancers[1].mSphereBatch->drawInstanced(dancers[1].jointSize);
    }
    if(isDancer3){
        dancers[2].mSphereBatch->drawInstanced(dancers[2].jointSize);
    }
    if(isDancer4){
        dancers[3].mSphereBatch->drawInstanced(dancers[3].jointSize);
    }
    if(isDancer5){
        dancers[4].mSphereBatch->drawInstanced(dancers[4].jointSize);
    }
    
    gl::setMatrices( mCamera );
    if (showGrid)renderScene();
    
    Color( dancerColor[0], dancerColor[1], dancerColor[2] );
    if(markersActive)   mSphereBatch->drawInstanced( jointList.size() );
    if(ribbonsActive)   drawRibbons();
    if(trailsActive)    handTrail.render(dancerColor);
    if(MocapSkeletonActive) {
        multipledancers=false;
        skeleton.rendermocap(true);
    }
    if(NewSkeletonActive) {
        multipledancers=false;
        skeleton.rendernewske();
    }
}

//--------------------  KEY DOWN -----------------------------
void Choreo3DApp::keyDown( KeyEvent event ){
    //skeleton.pushone(vec3(200,200,0));
    //writeImage( getDocumentsDirectory() / "Cinder" / "screenshots"/ "CCL_images" / "saveImage_" /( toString( mCurrentFrame ) + ".png" ), copyWindowSurface() );
    
//    int i = event.getCode();
//    cinder::app::console()<<i-48<<std::endl;
//    skeleton.push(vec3(500,-300,500), i-48);
//    skeletongui.skeleton->push(vec3(500,-300,500), i-46);
}


//------------------- SETUP THE ENVIRONMENT / GRID -----------------------
void Choreo3DApp::setupEnviron( int xSize, int zSize, int spacing )
{
    CI_ASSERT( ( spacing <= xSize ) && ( spacing <= zSize ) );
    // Cut in half and adjust for spacing.
    xSize = ( ( xSize / 2 ) / spacing ) * spacing;
    zSize = ( ( zSize / 2 ) / spacing ) * spacing;
    
    const int xMax = xSize + spacing;
    const int zMax = zSize + spacing;
    gridColor = ColorA( 0.9f, 0.9f, 0.9f,0.1f);
    const ColorA black( 0, 0, 0, 1 );
    
    mGridMesh = gl::VertBatch::create( GL_LINES );
    
    // Add x lines.
    for( int xVal = -xSize; xVal < xMax; xVal += spacing ) {
        mGridMesh->color( gridColor );
        mGridMesh->vertex( (float)xVal, 0, (float)-zSize );
        mGridMesh->vertex( (float)xVal, 0, (float)zSize );
    }// end for each x dir line
    
    // Add z lines.
    for( int zVal = -zSize; zVal < zMax; zVal += spacing ) {
        mGridMesh->color( gridColor );
        mGridMesh->vertex( (float)xSize, 0, (float)zVal );
        mGridMesh->vertex( (float)-xSize, 0, (float)zVal );
    }// end for each z dir line
    
    //SETUP THE CAMERA
    mCamera.setEyePoint(vec3(500,1000,0));
    mCamera.lookAt( vec3( 5400, 3400, 5800 ), vec3( 0 ) );
    mCamera.setFarClip(20000);
}

/*------------------ RENDER THE SCENE ------------------------*/
void Choreo3DApp::renderScene(){
    mGridMesh->draw();
}

/*--------------------- SETUP SHADERS -----------------------*/
void Choreo3DApp::setupShader(){
    //CHOOSE BETWEEN solidShader AND mGlsl AS SHADERS FOR THE SPHERES
    //    gl::ScopedColor color( Color( 0, 1, 0 ) );
    //    solidShader = gl::getStockShader( color );
    mGlsl = gl::GlslProg::create( loadAsset( "shader.vert" ), loadAsset( "shader.frag" ) );
    mSphereBatch = gl::Batch::create( geom::Sphere(), mGlsl );
}

//-------------------------UPDATE RIBBONS--------------------------------
void Choreo3DApp::updateRibbons(){
    auto easing = 1.0f;
    int i = 0;
    for (auto &r:ribbons){
        auto target = framePositions[i];
        const auto no_data_value = -123456;
        if (glm::all(glm::greaterThan(target, vec3(no_data_value)))){
            r._target = target;
        }
        
        auto &points = r._spine;
        for (auto i = points.size() - 1; i > 0; i -= 1){
            auto &p1 = points.at(i);
            auto &p2 = points.at(i - 1);
            p1 += (p2 - p1) * easing;
        }
        auto &point = points.at(0);
        point += (r._target - point) * easing;
        i++;
    }
    
    for (auto &r: ribbons){
        r._triangles = sansumbrella::createRibbon(20.0f, ci::EaseInOutQuad(), mCamera.getEyePoint(), r._spine);
    }
}


//--------------------- DRAW RIBBONS ---------------------------------
void Choreo3DApp::drawRibbons(){
    for (auto &ribbon: ribbons){
        gl::enableAlphaBlending();
        gl::color(ribbonColor);
        gl::begin(GL_TRIANGLE_STRIP);
        for (auto &p : ribbon._triangles){
            gl::vertex(p);
        }
        gl::end();
    }
}




//------------------------ S E T U P  G U I ------------------------------
void Choreo3DApp::initGui(){
    ui::initialize(opts);
    camActive = true;   //CAMERA MOVEMENT ENABLED BY DEFAULT
    ribbonsActive = false;
    trailsActive = false;
    MocapSkeletonActive = true;
    NewSkeletonActive = true;
    markersActive = true;
    showGrid = true;
    multipledancers = false;
    
    paused = false;  //PAUSED ENABLED BY DEFAULT
    background = 0.3f;
    
    skeletongui = GUI_Skeleton( &skeleton );
}

//------------------------ D I S P L A Y  G U I -------------------------
void Choreo3DApp::updateGui(){
    skeletongui.updateGUISke( &NewSkeletonActive );
    //skeletongui.drawSkeimage( &NewSkeletonActive );
    
    //CREATE A WINDOW
    ui::ScopedWindow window( "Choreo3D", ImGuiWindowFlags_NoMove);
    ui::SetWindowPos(ImVec2(920.0, getWindowHeight()-550));
    ImVec2 const SkeSize = ImVec2(360,540.0);
    ui::SetWindowSize(SkeSize, 0);
    
    ImVec2 spacing = ImVec2(15.,10.);
    ui::PushStyleVar(ImGuiStyleVar_ItemSpacing, spacing);
    
    //PLAY / PAUSE DANCER
    ui::Checkbox("PAUSED", &paused);
    //SHOW GRID
    ui::SameLine();
    ui::Checkbox("SHOW FLOOR", &showGrid);
    ui::SameLine();    if( ui::Button( "RESET" ) )   reset();


    //CREATE A SLIDING BAR TO SET THE BACKGROUND COLOR
    ImGui::SliderInt("SPEED", &frameRate, 0, 60);
    
    ui::Spacing();
    //CREATE A SLIDING BAR TO SET THE BACKGROUND COLOR
    ui::ColorEdit3("BACKGROUND", &testBK[0] );
    
    ui::Spacing();
    
    //SHOW MOCAP MARKERS
    ui::Checkbox("MOCAP MARKERS", &markersActive);
    ui::ColorEdit4("MARKER COLOUR", &markerColour[0] );
    
    ui::Spacing();
 
    //RIBBONS
    ui::Checkbox("TRAILS", &ribbonsActive);
    ui::ColorEdit4("TRAIL COLOUR", &ribbonColor[0] );
    ui::Spacing();

    //SKELETON
    ui::Checkbox("ORIGINAL SKELETON", &MocapSkeletonActive);   ui::SameLine();
    ui::Checkbox("NEW SKELETON", &NewSkeletonActive);
    ImGui::TextWrapped("Turn off multiple dancer mode to see skeleton");
    
    ui::Spacing();
    
    //SHOW PATH
    ui::Checkbox("PATH", &trailsActive);
    ui::Spacing();
    
    
    ui::Checkbox("MULTIPLE DANCERS", &multipledancers);
    
    if(multipledancers){
        MocapSkeletonActive = false;
        NewSkeletonActive = false;
        
        ui::Checkbox("DANCER1", &isDancer1); ui::SameLine();
        ImGui::SliderInt("D1 TIME", &fpsDancer1, 1, 60);
        ui::Checkbox("DANCER2", &isDancer2); ui::SameLine();
        ImGui::SliderInt("D2 TIME", &fpsDancer2, 1, 60);
        ui::Checkbox("DANCER3", &isDancer3); ui::SameLine();
        ImGui::SliderInt("D3 TIME", &fpsDancer3, 1, 60);
        ui::Checkbox("DANCER4", &isDancer4); ui::SameLine();
        ImGui::SliderInt("D4 TIME", &fpsDancer4, 1, 60);
        ui::Checkbox("DANCER5", &isDancer5); ui::SameLine();
        ImGui::SliderInt("D5 TIME", &fpsDancer5, 1, 60);
    }else{
        isDancer1 = false;
        isDancer2 = false;
        isDancer3 = false;
        isDancer4 = false;
        isDancer5 = false;
    }

    ui::PopStyleVar();
}

void Choreo3DApp::reset(){
    gl::clear( ColorA::gray( background ) );
    FRAME_COUNT = 0;
    mCurrentFrame = 0;
    handTrail.prevPos = handTrail.positions[0];
    handTrail.positions.clear();
    
    // skeleton need to reset twice, 1st time align the last frame. next time align the 1st frame
    skeleton.reset();
    skeleton.reset();
}


CINDER_APP( Choreo3DApp, RendererGl(RendererGl::Options().msaa( 16 ) ), [&]( App::Settings *settings ) {
    settings->setWindowSize( 1280, 720 );
    //settings->setFullScreen();
} )
