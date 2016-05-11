#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Fbo.h"
#include "cinder/Log.h"
#include "cinder/gl/Batch.h"
#include "cinder/Rand.h"

using namespace ci;
using namespace ci::app;
using namespace std;

const int   PARTICLES = 256; //squared
const float PRESIMULATION_DELTA_TIME = 0.1;
const int   BASE_LIFETIME = 2;
const float MAX_DELTA_TIME = 0.2;
const int   SLICES = 128;
const int   OPACITY_TEXTURE_RESOLUTION = 1024;

gl::GlslProgRef loadShader( const std::string& filename )
{
    gl::GlslProgRef prog;
    try {
        prog = gl::GlslProg::create( loadAsset( filename+".vert"), loadAsset(filename+".frag") );
    } catch (const gl::GlslProgCompileExc& e) {
        console() << filename+" shader" << e.what() << endl;
    } catch (const gl::GlslProgLinkExc& e) {
        console() << filename+" shader" << e.what() << endl;
    }
    return prog;
}

class half_angle_slicingApp : public App {
public:
    void setup() override;
    void mouseDown( MouseEvent event ) override;
    void update() override;
    void draw() override;
    
    gl::BatchRef    mVertices, mParticleQuad, mFullScreen, mFloor;
    gl::FboRef      mSimulationFbo[2], mOpacityFbo;
    gl::GlslProgRef mSimulationShader, mRenderShader, mSortShader, mOpacityShader, mFloorShader, mBackgroundShader;
    gl::TextureRef  mSpawnTexture;
    
    CameraPersp     mCamera;
    
    int mCurrent = 0;
    int mMaxParticleCount = PARTICLES * PARTICLES;
    
    float mInitialTurbulance = 0.4f;
    float mPrevTime = 0.f;
    float mDeltaTime = 0.f;
    float mInitialSpeed = 1.0f;
    
    bool mFirstFrame = true;
    
    int mTotalSortSteps;
    int mSortStepsLeft;
    int mSortPass = -1;
    int mSortStage = -1;

    bool mFlipped = false;
    
    CameraOrtho mLightCamera;
    
};

void half_angle_slicingApp::setup()
{
    mSimulationShader = loadShader("simulation");
    mRenderShader = loadShader("render");
    mSortShader = loadShader("sort");
    mOpacityShader = loadShader("opacity");
    mBackgroundShader = loadShader("background");
    mFloorShader = loadShader("floor");
    
    gl::Texture::Format fmt;
    fmt.setWrap( GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE );
    fmt.setMinFilter( GL_NEAREST );
    fmt.setMagFilter( GL_NEAREST );
    fmt.setInternalFormat(GL_RGBA32F);
    fmt.setDataType(GL_FLOAT);
    
    mSimulationFbo[0] = gl::Fbo::create(PARTICLES, PARTICLES, gl::Fbo::Format().colorTexture(fmt).disableDepth() );
    mSimulationFbo[1] = gl::Fbo::create(PARTICLES, PARTICLES, gl::Fbo::Format().colorTexture(fmt).disableDepth() );
    
    {
        gl::ScopedFramebuffer fbo( mSimulationFbo[0] );
        gl::clear();
    }
    {
        gl::ScopedFramebuffer fbo( mSimulationFbo[1] );
        gl::clear();
    }
    
    mParticleQuad = gl::Batch::create(geom::Plane().size(vec2(PARTICLES)).origin(vec3(PARTICLES/2, PARTICLES/2, 0.)).normal(vec3(0,0,1)), mSimulationShader);
    
    
    gl::Texture::Format opacity_fmt;
    opacity_fmt.setWrap( GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE );
    opacity_fmt.setMagFilter( GL_LINEAR );
    opacity_fmt.setMagFilter( GL_LINEAR );
    opacity_fmt.setDataType(GL_UNSIGNED_BYTE);
    opacity_fmt.setInternalFormat(GL_RGBA);
    
    mOpacityFbo = gl::Fbo::create( OPACITY_TEXTURE_RESOLUTION, OPACITY_TEXTURE_RESOLUTION, gl::Fbo::Format().disableDepth().colorTexture(opacity_fmt) );
    
    vector<vec4> positions;
    vector<vec2> texCoords;
    vector<vec4> spawn;
    
    for( int y = 0; y < PARTICLES; y++ ){
        for( int x = 0; x < PARTICLES; x++ ){
            
            auto sphere_pt = randVec3();
            
            auto tc = vec2( (x + 0.5) / PARTICLES, (y + 0.5) / PARTICLES );
            auto spawndata = vec4( sphere_pt*.1f, (float)BASE_LIFETIME + randFloat() * 2.f );
            
            spawn.push_back(spawndata);
            positions.push_back(vec4(tc,0.,1.));
            texCoords.push_back(tc);
            
        }
    }
    
    geom::BufferLayout pos_layout, tc_layout;
    
    auto pos_buffer = gl::Vbo::create(GL_ARRAY_BUFFER, sizeof(vec4)*positions.size(), positions.data(), GL_DYNAMIC_DRAW);
    
    pos_layout.append(geom::POSITION, 4, 0, 0);
    
    auto tc_buffer = gl::Vbo::create(GL_ARRAY_BUFFER, sizeof(vec2)*texCoords.size(), texCoords.data(), GL_STATIC_DRAW);
    
    tc_layout.append(geom::TEX_COORD_0, 2, 0, 0);
    
    vector<pair<geom::BufferLayout, gl::VboRef>> buffers;
    
    buffers.push_back(make_pair(pos_layout, pos_buffer));
    buffers.push_back(make_pair(tc_layout, tc_buffer));

    auto mesh = gl::VboMesh::create( PARTICLES * PARTICLES, GL_POINTS, buffers );
    
    mVertices = gl::Batch::create( mesh, mRenderShader);

    mSpawnTexture = gl::Texture::create( Surface32f( (float*)spawn.data(), PARTICLES, PARTICLES, sizeof(vec4)*PARTICLES, SurfaceChannelOrder::RGBA ), fmt );
    
    mCamera.setPerspective(60, getWindowAspectRatio(), .1, 10000);
    mCamera.lookAt(vec3(-1,0,.5),vec3(0));
    
    mLightCamera.setOrtho(-5.0, 5.0, -5.0, 5.0, -50.0, 50.0);
   // mLightCamera.setViewDirection(vec3(0,-1,0));
    //mLightCamera.setWorldUp(vec3(0,1,0));
    mLightCamera.lookAt(vec3(0,5,0), vec3(0,0,0));
    
    
    mFullScreen = gl::Batch::create(geom::Plane().size(getWindowSize()).origin(vec3(getWindowCenter(),0.)).normal(vec3(0,0,1)), mBackgroundShader);
    
    mFloor = gl::Batch::create(geom::Plane().size(vec2(100,100)).origin(vec3(0.,-.25, 0. )).normal(vec3(0,1,0)), mFloorShader);
    
    mTotalSortSteps = (log2(mMaxParticleCount) * (log2(mMaxParticleCount) + 1)) / 2;
    mSortStepsLeft = mTotalSortSteps;
    mSortPass = -1;
    mSortStage = -1;
    
    gl::enable(GL_VERTEX_PROGRAM_POINT_SIZE_NV);
}

void half_angle_slicingApp::mouseDown( MouseEvent event )
{
}

void half_angle_slicingApp::update()
{
    mCamera.lookAt( vec3( cos(getElapsedSeconds()*.5)+.5, 0., sin(getElapsedSeconds()*.5)  ), vec3(.5, 0,0) );
   // mLightCamera.lookAt(vec3(sin(getElapsedSeconds()), sin(getElapsedSeconds()*.5)*5., cos(getElapsedSeconds()*.5)*5.),vec3(0), vec3(0,1,0));

    gl::disableAlphaBlending();
    //swap
    mCurrent = 1 - mCurrent;
    
    //delta time
    mDeltaTime = getElapsedSeconds() - mPrevTime;
    mPrevTime = getElapsedSeconds();
    
    
    if (mDeltaTime > MAX_DELTA_TIME) {
        mDeltaTime = 0;
    }
    
    bool flippedThisFrame = false; //if the order reversed this frame
    
    vec3 viewDirection = mCamera.getViewDirection();
    vec3 lightViewDirection = mLightCamera.getViewDirection();
    vec3 halfVector;
    
    if (dot(viewDirection, lightViewDirection) > 0.0) {
        
        halfVector = normalize( lightViewDirection + viewDirection );
        
        if (mFlipped) {
            flippedThisFrame = true;
        }
        
        mFlipped = false;
        
    } else {
        
        halfVector = normalize( lightViewDirection - viewDirection );
        
        if (!mFlipped) {
            flippedThisFrame = true;
        }
        
        mFlipped = true;
    }

    mParticleQuad->replaceGlslProg(mSimulationShader);

    {
        gl::ScopedFramebuffer fbo( mSimulationFbo[mCurrent] );
        gl::ScopedMatrices pushMatrix;
        gl::ScopedViewport view( vec2(0), mSimulationFbo[mCurrent]->getSize() );
        gl::setMatricesWindow(mSimulationFbo[mCurrent]->getSize());
        
        gl::ScopedTextureBind prevPositions( mSimulationFbo[1-mCurrent]->getColorTexture(), 0 );
        gl::ScopedTextureBind spawnTex( mSpawnTexture, 1 );
        
        for (int i = 0; i < (mFirstFrame ? BASE_LIFETIME / PRESIMULATION_DELTA_TIME : 1); ++i) {
            
            auto glsl = mParticleQuad->getGlslProg();
            
            glsl->uniform( "uResolution", vec2(PARTICLES) );
            glsl->uniform( "uDeltaTime", mFirstFrame ? PRESIMULATION_DELTA_TIME : mDeltaTime * mInitialSpeed );
            glsl->uniform( "uTime", mFirstFrame ? PRESIMULATION_DELTA_TIME : (float)getElapsedSeconds() );
            glsl->uniform( "uParticleTexture", 0);
            glsl->uniform( "uSpawnTexture", 1);
            glsl->uniform( "uPersistence", mInitialTurbulance );
            
            mParticleQuad->draw();
        }
    }
    
    mFirstFrame = false;
    
    if (flippedThisFrame) { //if the order reversed this frame sort everything
        mSortPass = -1;
        mSortStage = -1;
        mSortStepsLeft = mTotalSortSteps;
    }
    
    
    mParticleQuad->replaceGlslProg(mSortShader);
    
    {
        
        for (int i = 0; i < ( mTotalSortSteps ); ++i) {
            
            mCurrent = 1 - mCurrent;
            
            gl::ScopedFramebuffer fbo( mSimulationFbo[mCurrent] );
            
            gl::ScopedMatrices pushMatrix;
            
            gl::setMatricesWindow( mSimulationFbo[mCurrent]->getSize() );
            gl::ScopedViewport view( vec2(0), mSimulationFbo[mCurrent]->getSize() );
            
            mSortPass--;
            
            if (mSortPass < 0) {
                mSortStage++;
                mSortPass = mSortStage;
            }
            
            gl::ScopedTextureBind particleTex( mSimulationFbo[1-mCurrent]->getColorTexture(), 0 );
            
            auto glsl = mParticleQuad->getGlslProg();
            glsl->uniform( "uDataTexture", 0);
            glsl->uniform( "uResolution", vec2(PARTICLES) );
            glsl->uniform( "uPass", float(1 << mSortPass) );
            glsl->uniform( "uStage", float(1 << mSortStage) );
            glsl->uniform( "uHalfVector", halfVector );
            
            mParticleQuad->draw();
            
            mSortStepsLeft--;
            
            if (mSortStepsLeft == 0) {
                mSortStepsLeft = mTotalSortSteps;
                mSortPass = -1;
                mSortStage = -1;
            }
        }
    }


    
    if(getElapsedFrames() % 60 ==0 )console() << getAverageFps() << endl;

}

void half_angle_slicingApp::draw()
{
    gl::clear( ColorA( 0.0, 0.0, 0.0, 0 ) );

    gl::enableAlphaBlending();
    
    {
        gl::ScopedFramebuffer fbo( mOpacityFbo );
        gl::clear( ColorA(0,0,0,0) );
    }
    
    gl::ScopedTextureBind particles( mSimulationFbo[mCurrent]->getColorTexture(), 0 );
    
    for( int i = 0; i < SLICES; i++ ){
        
        
        mVertices->replaceGlslProg(mRenderShader);
        
        ///RENDER PARTICLES
        
        {
            gl::ScopedMatrices pushMatrix;
            gl::setMatrices(mCamera);
            gl::ScopedViewport view( vec2(0), getWindowSize() );
            
            gl::ScopedTextureBind opacity( mOpacityFbo->getColorTexture(), 1 );
            
            auto glsl = mVertices->getGlslProg();
            glsl->uniform( "uParticleTexture", 0 );
            glsl->uniform( "uOpacityTexture", 1 );
            glsl->uniform( "uLightViewProjection", mLightCamera.getProjectionMatrix() * mLightCamera.getViewMatrix() );
            glsl->uniform( "uParticleDiameter", 0.02f );
            glsl->uniform( "uScreenWidth", (float)getWindowWidth() );
            glsl->uniform( "uParticleAlpha", .4f );
            glsl->uniform( "uParticleColor", vec3(1,0,.5) );
            
            if( mFlipped ){
                
                gl::ScopedBlend blend( GL_ONE, GL_ONE_MINUS_SRC_ALPHA );
                
                mVertices->draw( i * (mMaxParticleCount / SLICES), mMaxParticleCount / SLICES);
                
            }else{
                
                gl::ScopedBlend blend(  GL_ONE_MINUS_DST_ALPHA, GL_ONE );
                
                mVertices->draw( i * (mMaxParticleCount / SLICES), mMaxParticleCount / SLICES);
                
            }
            
        }
        
        ///OPACTIY PASS : FROM LIGHT
        mVertices->replaceGlslProg(mOpacityShader);
        
        {
            
            gl::ScopedFramebuffer fbo( mOpacityFbo );
            gl::ScopedMatrices pushMatrix;
            
            gl::setMatrices(mLightCamera);
            gl::ScopedViewport view( vec2(0), mOpacityFbo->getSize() );
            
            auto glsl = mVertices->getGlslProg();
            
            glsl->uniform( "uParticleTexture", 0 );
            glsl->uniform( "uParticleDiameter", .02f );
            glsl->uniform( "uScreenWidth", (float)mOpacityFbo->getWidth() );
            glsl->uniform( "uParticleAlpha", .4f );
            
            gl::ScopedBlend blend( GL_ONE, GL_ONE_MINUS_SRC_ALPHA );
            
            mVertices->draw( i * (mMaxParticleCount / SLICES), mMaxParticleCount / SLICES);
            
        }
        
    }
    
    {
        gl::ScopedMatrices push;
        gl::setMatrices(mCamera);
        gl::ScopedViewport view( vec2(0,0), getWindowSize() );
        gl::ScopedTextureBind opacity( mOpacityFbo->getColorTexture(), 2 );
        auto glsl = mFloor->getGlslProg();
        glsl->uniform( "uLightViewProjection", mLightCamera.getProjectionMatrix() * mLightCamera.getViewMatrix()  );
        glsl->uniform( "uOpacityTexture", 2 );
        
        gl::ScopedBlend blend(  GL_ONE_MINUS_DST_ALPHA, GL_ONE );
        
        mFloor->draw();
    }


    {
        gl::ScopedMatrices push;
        gl::setMatricesWindow(getWindowSize());
        gl::ScopedViewport view( vec2(0), getWindowSize() );
        
        gl::ScopedBlend blend(  GL_ONE_MINUS_DST_ALPHA, GL_ONE );

        mFullScreen->draw();
    }

    
}

CINDER_APP( half_angle_slicingApp, RendererGl, [&](half_angle_slicingApp::Settings*settigns){ settigns->setFullScreen(); } )
