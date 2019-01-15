#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/CameraUi.h"
#include "cinder/Log.h"

#include "AssetManager.h"
#include "MiniConfig.h"

#include <time.h>

using namespace ci;
using namespace ci::app;
using namespace std;

class ShaderToyApp : public App
{
  public:

      void mouseDown(MouseEvent event) override
      {
          mMouse.x = (float)event.getPos().x;
          mMouse.y = (float)event.getPos().y;
          mMouse.z = (float)event.getPos().x;
          mMouse.w = (float)event.getPos().y;
      }

      void mouseDrag(MouseEvent event) override
      {
          mMouse.x = (float)event.getPos().x;
          mMouse.y = (float)event.getPos().y;
      }

    void setup() override
    {
        log::makeLogger<log::LoggerFile>();
        
        auto aabb = am::triMesh(MESH_NAME)->calcBoundingBox();
        mCam.lookAt(aabb.getMax() * 2.0f, aabb.getCenter());
        mCamUi = CameraUi( &mCam, getWindow(), -1 );
        
        createConfigUI({200, 200});
        gl::enableDepth();

        getWindow()->getSignalResize().connect([&] {
            APP_WIDTH = getWindowWidth();
            APP_HEIGHT = getWindowHeight();
            mCam.setAspectRatio( getWindowAspectRatio() );
        });

        getWindow()->getSignalKeyUp().connect([&](KeyEvent& event) {
            if (event.getCode() == KeyEvent::KEY_ESCAPE) quit();
        });
        
#if 1
        try {
            std::string vs = am::str(VS_NAME);
            std::string fs = am::str("common/shadertoy.inc") + am::str(FS_NAME);

            mGlslProg = gl::GlslProg::create(gl::GlslProg::Format().vertex(vs).fragment(fs));
        }
        catch (const std::exception &e) {
            // Uhoh, something went wrong, but it's not fatal.
            CI_LOG_EXCEPTION("Failed to compile the shader: ", e);
        }
#else
        mGlslProg = am::glslProg(VS_NAME, FS_NAME);
#endif
        mChannel0 = am::texture2d(TEX0_NAME);
        mChannel1 = am::texture2d(TEX1_NAME);
        mChannel2 = am::texture2d(TEX2_NAME);
        mChannel3 = am::texture2d(TEX3_NAME);

        getWindow()->getSignalDraw().connect([&] {
            gl::clear();
        
            // Calculate shader parameters.
            vec3  iResolution(vec2(getWindowSize()), 1);
            float iGlobalTime = (float)getElapsedSeconds();
            float iChannelTime0 = (float)getElapsedSeconds();
            float iChannelTime1 = (float)getElapsedSeconds();
            float iChannelTime2 = (float)getElapsedSeconds();
            float iChannelTime3 = (float)getElapsedSeconds();
            vec3  iChannelResolution0 = mChannel0 ? vec3(mChannel0->getSize(), 1) : vec3(1);
            vec3  iChannelResolution1 = mChannel1 ? vec3(mChannel1->getSize(), 1) : vec3(1);
            vec3  iChannelResolution2 = mChannel2 ? vec3(mChannel2->getSize(), 1) : vec3(1);
            vec3  iChannelResolution3 = mChannel3 ? vec3(mChannel3->getSize(), 1) : vec3(1);

            time_t now = time(0);
            tm *   t = gmtime(&now);
            vec4   iDate(float(t->tm_year + 1900), float(t->tm_mon + 1), float(t->tm_mday), float(t->tm_hour * 3600 + t->tm_min * 60 + t->tm_sec));

            // Set shader uniforms.
            mGlslProg->uniform("iResolution", iResolution);
            mGlslProg->uniform("iGlobalTime", iGlobalTime);
            mGlslProg->uniform("iChannelTime[0]", iChannelTime0);
            mGlslProg->uniform("iChannelTime[1]", iChannelTime1);
            mGlslProg->uniform("iChannelTime[2]", iChannelTime2);
            mGlslProg->uniform("iChannelTime[3]", iChannelTime3);
            mGlslProg->uniform("iChannelResolution[0]", iChannelResolution0);
            mGlslProg->uniform("iChannelResolution[1]", iChannelResolution1);
            mGlslProg->uniform("iChannelResolution[2]", iChannelResolution2);
            mGlslProg->uniform("iChannelResolution[3]", iChannelResolution3);
            mGlslProg->uniform("iMouse", mMouse);
            mGlslProg->uniform("iChannel0", 0);
            mGlslProg->uniform("iChannel1", 1);
            mGlslProg->uniform("iChannel2", 2);
            mGlslProg->uniform("iChannel3", 3);
            mGlslProg->uniform("iDate", iDate);

            gl::ScopedTextureBind tex0(mChannel0, 0);
            gl::ScopedTextureBind tex1(mChannel1, 1);
            gl::ScopedTextureBind tex2(mChannel2, 2);
            gl::ScopedTextureBind tex3(mChannel3, 3);
            gl::ScopedGlslProg glsl(mGlslProg);

#if 0
            //gl::setMatrices( mCam );
            gl::draw(am::vboMesh(MESH_NAME));
#else
            gl::drawSolidRect(Rectf(0, (float)getWindowHeight(), (float)getWindowWidth(), 0));
#endif
        });
    }
    
private:
    CameraPersp         mCam;
    CameraUi            mCamUi;
    gl::GlslProgRef     mGlslProg;
    //! Texture slots for our shader, based on ShaderToy.
    gl::TextureRef mChannel0;
    gl::TextureRef mChannel1;
    gl::TextureRef mChannel2;
    gl::TextureRef mChannel3;
    //! Our mouse position: xy = current position while mouse down, zw = last click position.
    vec4 mMouse;
};

CINDER_APP( ShaderToyApp, RendererGl, [](App::Settings* settings) {
    readConfig();
    settings->setWindowSize(APP_WIDTH, APP_HEIGHT);
    settings->setMultiTouchEnabled(false);
} )
