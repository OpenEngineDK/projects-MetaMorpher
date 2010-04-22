// Main setup for the OpenEngine MetaMopher project.
// -------------------------------------------------------------------
// Copyright (C) 2008 OpenEngine.dk (See AUTHORS) 
// 
// This program is free software; It is covered by the GNU General 
// Public License version 2 or any later version. 
// See the GNU General Public License for more details (see LICENSE). 
//--------------------------------------------------------------------

// OpenEngine stuff
//#include <Meta/Config.h>

// Core structures
#include <Core/Engine.h>
#include <Core/GLUTEngine.h>

// Display structures
#include <Display/IFrame.h>
#include <Display/FollowCamera.h>
#include <Display/Frustum.h>
#include <Display/InterpolatedViewingVolume.h>
#include <Display/ViewingVolume.h>
#include <Display/IEnvironment.h>
#include <Display/HUD.h>

// SDL implementation
// #include <Display/SDLFrame.h>
// #include <Devices/SDLInput.h>

// GLUT implementation
#include <Display/GLUTEnvironment.h>

// OpenGL rendering implementation
#include <Renderers/OpenGL/LightRenderer.h>
#include <Renderers/OpenGL/Renderer.h>
// #include <Renderers/OpenGL/FBOBufferedRenderer.h>
// #include <Renderers/OpenGL/GLCopyBufferedRenderer.h>
#include <Renderers/OpenGL/RenderingView.h>
#include <Renderers/TextureLoader.h>
#include <Display/OpenGL/TextureCanvas.h>
#include <Renderers/OpenGL/CompositeCanvas.h>
#include <Renderers/OpenGL/ColorStereoRenderer.h>

// Resources
#include <Resources/IModelResource.h>
#include <Resources/File.h>
#include <Resources/DirectoryManager.h>
#include <Resources/ResourceManager.h>
// OBJ and TGA plugins
#include <Resources/ITexture2D.h>
#include <Resources/SDLImage.h>
#include <Resources/OBJResource.h>

// Scene structures
#include <Scene/ISceneNode.h>
#include <Scene/SceneNode.h>
#include <Scene/GeometryNode.h>
#include <Scene/TransformationNode.h>

// Utilities and logger
#include <Logging/Logger.h>
#include <Logging/StreamLogger.h>
#include <Scene/DotVisitor.h>

// OERacer utility files
#include <Utils/QuitHandler.h>

#include <Utils/Billboard.h>

#include <Animations/MetaMorpher.h>
#include <Animations/TransformationNodeMorpher.h>

#include "KeyHandler.h"

// Additional namespaces
using namespace OpenEngine;
using namespace OpenEngine::Animation;
using namespace OpenEngine::Core;
using namespace OpenEngine::Logging;
using namespace OpenEngine::Devices;
using namespace OpenEngine::Display;
using namespace OpenEngine::Renderers::OpenGL;
using namespace OpenEngine::Display::OpenGL;
using namespace OpenEngine::Renderers;
using namespace OpenEngine::Resources;
using namespace OpenEngine::Utils;

// Configuration structure to pass around to the setup methods
struct Config {
    IEngine&              engine;
    IFrame&               frame;
    CompositeCanvas*      split;
    IViewingVolume*       viewingvolume;
    FollowCamera*         camera;
    Frustum*              frustum;
    IRenderer*            renderer;
    IMouse*               mouse;
    IKeyboard*            keyboard;
    ISceneNode*           scene;
    TextureLoader*        textureLoader;
    IEnvironment*         env;
    Config(IEngine& engine, IEnvironment* env, IFrame& frame)
        : engine(engine)
        , frame(frame)
        , split(NULL)
        , viewingvolume(NULL)
        , camera(NULL)
        , frustum(NULL)
        , renderer(NULL)
        , mouse(NULL)
        , keyboard(NULL)
        , scene(NULL)
        , textureLoader(NULL)
        , env(env)
    {}
};

// Forward declaration of the setup methods
void SetupResources(Config&);
void SetupDevices(Config&);
void SetupDisplay(Config&);
void SetupRendering(Config&);
void SetupScene(Config&);
void SetupDebugging(Config&);
void SetupSound(Config&);

int main(int argc, char** argv) {

    // Setup logging facilities.
    Logger::AddLogger(new StreamLogger(&std::cout));

    // Create an engine and config object
    IEngine* engine = new GLUTEngine();
    IEnvironment* env = new GLUTEnvironment(800, 600, 16);//FRAME_FULLSCREEN);
    Config config(*engine, env, env->CreateFrame());
    
    // Setup the engine
    SetupResources(config);
    SetupDisplay(config);
    SetupDevices(config);
    SetupRendering(config);
    SetupScene(config);

    // Possibly add some debugging stuff
    SetupDebugging(config);

    logger.info << "main scene: " << config.frame.GetScene() << logger.end;

    // Start up the engine.
    engine->Start();

    // release event system
    // post condition: scene and modules are not processed
    delete engine;

    delete config.scene;

    // Return when the engine stops.
    return EXIT_SUCCESS;
}

void SetupResources(Config& config) {
    // set the resources directory
    // @todo we should check that this path exists
    // set the resources directory
    string resources = "projects/MetaMorpher/data/";
    DirectoryManager::AppendPath(resources);

    // load resource plug-ins
    ResourceManager<IModelResource>::AddPlugin(new OBJPlugin());
}

void SetupDisplay(Config& config) {
    if (config.viewingvolume != NULL ||
        config.camera        != NULL ||
        config.frustum       != NULL //||
        )
        throw Exception("Setup display dependencies are not satisfied.");

    config.engine.InitializeEvent().Attach(*config.env);
    config.engine.ProcessEvent().Attach(*config.env);
    config.engine.DeinitializeEvent().Attach(*config.env);
    
    config.viewingvolume = new InterpolatedViewingVolume(*(new ViewingVolume()));
    config.camera        = new FollowCamera( *config.viewingvolume );

    config.camera->SetPosition(Vector<3,float>(0,0,20));
    config.camera->LookAt(Vector<3,float>(0,0,0));

}

void SetupDevices(Config& config) {
    if (config.keyboard != NULL ||
        config.mouse    != NULL)
        throw Exception("Setup devices dependencies are not satisfied.");
    // Create the mouse and keyboard input modules
    config.keyboard = config.env->GetKeyboard();
    config.mouse = config.env->GetMouse();

    // Bind the quit handler
    QuitHandler* quit_h = new QuitHandler(config.engine);
    config.keyboard->KeyEvent().Attach(*quit_h);

    config.split = new CompositeCanvas();
    KeyHandler* kh = new KeyHandler(config.frame, *config.split);
    config.keyboard->KeyEvent().Attach(*kh);
}

void SetupRendering(Config& config) {
    if (config.renderer != NULL ||
        config.camera == NULL )
        throw Exception("Setup renderer dependencies are not satisfied.");

    RenderStateNode* renderStateNode = new RenderStateNode();
    renderStateNode->DisableOption(RenderStateNode::LIGHTING);
    config.scene = renderStateNode;

    ISceneNode* scene = new RenderStateNode();

    // config.frame.Attach(*config.split);
    config.frame.SetViewingVolume(config.camera);

    IRenderer* textureRenderer = new Renderer();
    textureRenderer->SetBackgroundColor(Vector<4,float>(0.4,0,0.4,1));
    TextureCanvas* skinTextureFrame = new TextureCanvas(config.frame);
    ITexture2DPtr skinTexture = skinTextureFrame->GetTexture();
    skinTextureFrame->SetScene(config.scene);
    skinTextureFrame->SetViewingVolume(config.camera);

    IRenderer* sceneRenderer = new Renderer();
    TextureCanvas* sceneTextureFrame = new TextureCanvas(config.frame);
    ITexture2DPtr sceneTexture = sceneTextureFrame->GetTexture();
    sceneRenderer->SetBackgroundColor(Vector<4,float>(0.2,0.2,0.2,1));
    sceneTextureFrame->SetScene(scene);
    sceneTextureFrame->SetViewingVolume(config.camera);
    
    TransformationNode* board = Billboard::
        CreateTextureBillboard( skinTexture, 0.025 );
    board->Move(-10,-10,-20);
    board->Rotate(0,Math::PI/8,0);
    scene->AddNode(board);

    TransformationNode* mirror = Billboard::
        CreateTextureBillboard( sceneTexture, 0.025 );
    mirror->Move(10,-10,-20);
    mirror->Rotate(0,-Math::PI/8,0);
    scene->AddNode(mirror);

    // setup top renderer, which renderers the scene
    config.renderer = sceneRenderer; // for the setupScene to set scene
    // Setup a rendering view for both renderers
    RenderingView* rv = new RenderingView();
    textureRenderer->ProcessEvent().Attach(*rv);
    sceneRenderer->ProcessEvent().Attach(*rv);

    // Add rendering initialization tasks
    config.textureLoader = new TextureLoader(*textureRenderer);
    textureRenderer->PreProcessEvent().Attach(*config.textureLoader);
    textureRenderer->PreProcessEvent()
        .Attach( *(new LightRenderer()) );
 

    ColorStereoRenderer* stereo = new ColorStereoRenderer();

    // and all the wiring ...
    config.frame.InitializeEvent().Attach(*textureRenderer);
    config.frame.DeinitializeEvent().Attach(*textureRenderer);
    config.frame.InitializeEvent().Attach(*sceneRenderer);
    config.frame.DeinitializeEvent().Attach(*sceneRenderer);
    config.frame.InitializeEvent().Attach(*skinTextureFrame);
    config.frame.DeinitializeEvent().Attach(*skinTextureFrame);
    config.frame.InitializeEvent().Attach(*sceneTextureFrame);
    config.frame.DeinitializeEvent().Attach(*sceneTextureFrame);
    config.frame.InitializeEvent().Attach(*stereo);
    config.frame.DeinitializeEvent().Attach(*stereo);

    config.frame.RedrawEvent().Attach(*skinTextureFrame);
    config.frame.RedrawEvent().Attach(*sceneTextureFrame);
    skinTextureFrame->RedrawEvent().Attach(*textureRenderer);
    sceneTextureFrame->RedrawEvent().Attach(*stereo);
    // sceneTextureFrame->RedrawEvent().Attach(*sceneRenderer);
    stereo->RedrawEvent().Attach(*sceneRenderer);
    // TextureCanvas* topFrame = sceneTextureFrame;
    // config.split->AddCanvas(topFrame);
    // config.split->AddCanvas(topFrame);
    // config.split->AddCanvas(topFrame);
    // config.split->AddCanvas(topFrame);
    // config.split->AddCanvas(topFrame);
    // config.split->AddCanvas(topFrame);
    // config.split->AddCanvas(topFrame);
    // config.split->AddCanvas(topFrame);
    // config.split->AddCanvas(topFrame);
}

void SetupScene(Config& config) {
    if (config.scene  == NULL ||
        config.mouse  == NULL ||
        config.keyboard == NULL)
        throw Exception("Setup scene dependencies are not satisfied.");

    // Create a root scene node


    // Supply the scene to the renderer
    // config.renderer->SetSceneRoot(config.scene);
    
    //todo
    

    // load two textures from file.
    // create combo texture, add to texture loader - RELOAD_ALLWAYS
    // set up billboard with combo texture

    // create and add mopher to process
    // could: cycle(loop), "scan", default(stand still after timout) 

    // attach combo texture to mopher
    

    TransformationNode* left = new TransformationNode();
    left->SetPosition(Vector<3,float>(-10,0,0));

    TransformationNode* topCenter = new TransformationNode();
    topCenter->SetPosition(Vector<3,float>(0,7,0));
    //topCenter->SetRotation(Quaternion<float>(Math::PI/2,0,Math::PI/2));

    TransformationNode* right = new TransformationNode();
    right->SetPosition(Vector<3,float>(10,0,0));
    //right->SetRotation(Quaternion<float>(Math::PI,0,Math::PI));

    TransformationNode* bottomCenter = new TransformationNode();
    bottomCenter->SetPosition(Vector<3,float>(0,-7,0));
    //bottomCenter->SetRotation(Quaternion<float>(Math::PI/2,0,Math::PI/2));

    TransformationNodeMorpher* tmorpher =
      new TransformationNodeMorpher();
    
    MetaMorpher<TransformationNode>* metamorpher =
      new MetaMorpher<TransformationNode>
      (tmorpher,LOOP);
    config.engine.ProcessEvent().Attach(*metamorpher);
    metamorpher->Add(left, new Utils::Time(0));
    metamorpher->Add(topCenter, new Utils::Time(3000000));
    metamorpher->Add(right, new Utils::Time(6000000));
    metamorpher->Add(bottomCenter, new Utils::Time(9000000));
    metamorpher->Add(left, new Utils::Time(12000000));
    
    TransformationNode* trans = metamorpher->GetObject();
    config.scene->AddNode(trans);

    FaceSet* faces = new FaceSet();

    Vector<3,float> p1(1,1,0);
    Vector<3,float> p2(1,-1,0);
    Vector<3,float> p3(-1,-1,0);
    //Vector<3,float> p4(-1,1,0);
    Vector<3,float> n(0,0,1);

    FacePtr face1 = FacePtr(new Face(p1,p2,p3,n,n,n));
    
    faces->Add(face1);

    GeometryNode* billboard = new GeometryNode(faces);

    trans->AddNode(billboard);
}

void SetupDebugging(Config& config) {
    // Visualization of the frustum
    if (config.frustum != NULL) {
        config.frustum->VisualizeClipping(true);
        config.scene->AddNode(config.frustum->GetFrustumNode());
    }

    ofstream dotfile("scene.dot", ofstream::out);
    if (!dotfile.good()) {
        logger.error << "Can not open 'scene.dot' for output"
                     << logger.end;
    } else {
        DotVisitor dot;
        dot.Write(*config.scene, &dotfile);
        logger.info << "Saved scene graph to 'scene.dot'"
                    << logger.end
                    << "To create a SVG image run: "
                    << "dot -Tsvg scene.dot > scene.svg"
                    << logger.end;
    }
}

