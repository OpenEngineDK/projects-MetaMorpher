// Main setup for the OpenEngine MetaMopher project.
// -------------------------------------------------------------------
// Copyright (C) 2008 OpenEngine.dk (See AUTHORS) 
// 
// This program is free software; It is covered by the GNU General 
// Public License version 2 or any later version. 
// See the GNU General Public License for more details (see LICENSE). 
//--------------------------------------------------------------------

// OpenEngine stuff
#include <Meta/Config.h>

// Core structures
#include <Core/Engine.h>

// Display structures
#include <Display/IFrame.h>
#include <Display/FollowCamera.h>
#include <Display/Frustum.h>
#include <Display/InterpolatedViewingVolume.h>
#include <Display/ViewingVolume.h>
// SDL implementation
#include <Display/HUD.h>
#include <Display/SDLFrame.h>
#include <Devices/SDLInput.h>

// OpenGL rendering implementation
#include <Renderers/OpenGL/LightRenderer.h>
#include <Renderers/OpenGL/Renderer.h>
#include <Renderers/OpenGL/FBOBufferedRenderer.h>
#include <Renderers/OpenGL/GLCopyBufferedRenderer.h>
#include <Renderers/OpenGL/RenderingView.h>
#include <Renderers/TextureLoader.h>

// Resources
#include <Resources/IModelResource.h>
#include <Resources/File.h>
#include <Resources/DirectoryManager.h>
#include <Resources/ResourceManager.h>
// OBJ and TGA plugins
#include <Resources/ITextureResource.h>
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

#include <Animation/MetaMorpher.h>
#include <Animation/TransformationNodeMorpher.h>

// Additional namespaces
using namespace OpenEngine;
using namespace OpenEngine::Animation;
using namespace OpenEngine::Core;
using namespace OpenEngine::Logging;
using namespace OpenEngine::Devices;
using namespace OpenEngine::Display;
using namespace OpenEngine::Renderers::OpenGL;
using namespace OpenEngine::Renderers;
using namespace OpenEngine::Resources;
using namespace OpenEngine::Utils;

// Configuration structure to pass around to the setup methods
struct Config {
    IEngine&              engine;
    IFrame*               frame;
    Viewport*             viewport;
    IViewingVolume*       viewingvolume;
    FollowCamera*         camera;
    Frustum*              frustum;
    IRenderer*            renderer;
    IMouse*               mouse;
    IKeyboard*            keyboard;
    ISceneNode*           scene;
    TextureLoader*        textureLoader;
    Config(IEngine& engine)
        : engine(engine)
        , frame(NULL)
        , viewport(NULL)
        , viewingvolume(NULL)
        , camera(NULL)
        , frustum(NULL)
        , renderer(NULL)
        , mouse(NULL)
        , keyboard(NULL)
        , scene(NULL)
        , textureLoader(NULL)
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
    Engine* engine = new Engine();
    Config config(*engine);

    // Setup the engine
    SetupResources(config);
    SetupDisplay(config);
    SetupDevices(config);
    SetupRendering(config);
    SetupScene(config);

    // Possibly add some debugging stuff
    SetupDebugging(config);

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
    if (config.frame         != NULL ||
        config.viewingvolume != NULL ||
        config.camera        != NULL ||
        config.frustum       != NULL ||
        config.viewport      != NULL)
        throw Exception("Setup display dependencies are not satisfied.");

    //config.frame         = new SDLFrame(1440, 900, 32, FRAME_FULLSCREEN);
    config.frame         = new SDLFrame(800, 600, 32);
    config.viewingvolume = new InterpolatedViewingVolume(*(new ViewingVolume()));
    config.camera        = new FollowCamera( *config.viewingvolume );
    //config.frustum       = new Frustum(*config.camera, 20, 3000);

    config.camera->SetPosition(Vector<3,float>(0,0,20));
    config.camera->LookAt(Vector<3,float>(0,0,0));

    config.viewport      = new Viewport(*config.frame);
    config.viewport->SetViewingVolume(config.camera);

    config.engine.InitializeEvent().Attach(*config.frame);
    config.engine.ProcessEvent().Attach(*config.frame);
    config.engine.DeinitializeEvent().Attach(*config.frame);
}

void SetupDevices(Config& config) {
    if (config.keyboard != NULL ||
        config.mouse    != NULL)
        throw Exception("Setup devices dependencies are not satisfied.");
    // Create the mouse and keyboard input modules
    SDLInput* input = new SDLInput();
    config.keyboard = input;
    config.mouse = input;

    // Bind the quit handler
    QuitHandler* quit_h = new QuitHandler(config.engine);
    config.keyboard->KeyEvent().Attach(*quit_h);

    // Bind to the engine for processing time
    config.engine.InitializeEvent().Attach(*input);
    config.engine.ProcessEvent().Attach(*input);
    config.engine.DeinitializeEvent().Attach(*input);
}

IBufferedRenderer* CreateBufferedRenderer(Display::Viewport* vp) {
    return new GLCopyBufferedRenderer(vp);
    //return new FBOBufferedRenderer(vp);
}

void SetupRendering(Config& config) {
    if (config.viewport == NULL ||
        config.renderer != NULL ||
        config.camera == NULL )
        throw Exception("Setup renderer dependencies are not satisfied.");

    IBufferedRenderer* textureRenderer = CreateBufferedRenderer(config.viewport);
    textureRenderer->SetBackgroundColor(Vector<4,float>(0,0,0,1));
    ITextureResourcePtr skinTexture = textureRenderer->GetColorBuffer();

    // Create a renderer, thats renderers the texture for the quad
    IBufferedRenderer* sceneRenderer = CreateBufferedRenderer(config.viewport);
    sceneRenderer->SetBackgroundColor(Vector<4,float>(1,0,0,1));

    ISceneNode* scene = new RenderStateNode();

    // create and setup the quad bill board
    ITextureResourcePtr sceneTexture = sceneRenderer->GetColorBuffer();
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
    config.renderer = textureRenderer; // for the setupScene to set scene
    sceneRenderer->SetSceneRoot(scene);

    // Setup a rendering view for both renderers
    // this only holds perspective
    RenderingView* rv = new RenderingView(*config.viewport);
    textureRenderer->ProcessEvent().Attach(*rv);
    sceneRenderer->ProcessEvent().Attach(*rv);

    // Add rendering initialization tasks
    config.textureLoader = new TextureLoader(*textureRenderer);
    textureRenderer->PreProcessEvent().Attach(*config.textureLoader);

    textureRenderer->PreProcessEvent()
      .Attach( *(new LightRenderer(*config.camera)) );

    // needs to be first for the mirror effect to work
    config.engine.InitializeEvent().Attach(*textureRenderer);
    config.engine.ProcessEvent().Attach(*textureRenderer);
    config.engine.DeinitializeEvent().Attach(*textureRenderer);

    config.engine.InitializeEvent().Attach(*sceneRenderer);
    config.engine.ProcessEvent().Attach(*sceneRenderer);
    config.engine.DeinitializeEvent().Attach(*sceneRenderer);

    Renderer* hudRenderer = new Renderer(config.viewport);
    hudRenderer->SetBackgroundColor(Vector<4,float>(0,1,0,1));
    hudRenderer->SetSceneRoot(new SceneNode());

    // to apply top renderer
    HUD* hud = new HUD();
    hudRenderer->PostProcessEvent().Attach( *hud );
    HUD::Surface* surface = hud->CreateSurface(sceneTexture);
    surface->SetPosition(HUD::Surface::RIGHT, HUD::Surface::TOP);

    config.engine.InitializeEvent().Attach(*hudRenderer);
    config.engine.ProcessEvent().Attach(*hudRenderer);
    config.engine.DeinitializeEvent().Attach(*hudRenderer);
    hudRenderer->ProcessEvent().Attach(*rv);
}

void SetupScene(Config& config) {
    if (config.scene  != NULL ||
        config.mouse  == NULL ||
        config.keyboard == NULL)
        throw Exception("Setup scene dependencies are not satisfied.");

    // Create a root scene node

    RenderStateNode* renderStateNode = new RenderStateNode();
    renderStateNode->DisableOption(RenderStateNode::LIGHTING);
    config.scene = renderStateNode;

    // Supply the scene to the renderer
    config.renderer->SetSceneRoot(config.scene);

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

