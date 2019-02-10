#include "cinder/FileWatcher.h"
#include "cinder/Log.h"
#include "cinder/Utilities.h"
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/params/Params.h"

#include "AssetManager.h"
#include "MiniConfig.h"

#include <time.h>

using namespace ci;
using namespace ci::app;
using namespace std;

class ShaderToyApp : public App
{
  public:
    vector<string> listToyFiles()
    {
        vector<string> files;
        auto assetRoot = getAssetPath("");
        for (auto& p : fs::directory_iterator(assetRoot))
        {
            auto ext = p.path().extension();
            if (ext == ".frag" || ext == ".fs" || ext == ".fragment")
            {
                auto filename = p.path().generic_string();
                filename.replace(filename.find(assetRoot.generic_string()),
                                 assetRoot.generic_string().size(),
                                 ""); // Left trim the assets prefix

                files.push_back(filename);
            }
        }

        return files;
    }

    void setup() override
    {
        log::makeLogger<log::LoggerFile>();

        gl::enableDepth();

        getWindow()->getSignalMove().connect([&] {
            APP_X = getWindowPosX();
            APP_Y = getWindowPosY();
        });

        getWindow()->getSignalResize().connect([&] {
            APP_WIDTH = getWindowWidth();
            APP_HEIGHT = getWindowHeight();
        });

        getWindow()->getSignalMouseDown().connect([&](MouseEvent event) {
            mMouse.x = (float)event.getPos().x;
            mMouse.y = (float)event.getPos().y;
            mMouse.z = (float)event.getPos().x;
            mMouse.w = (float)event.getPos().y;
        });

        getWindow()->getSignalMouseDrag().connect([&](MouseEvent event) {
            mMouse.x = (float)event.getPos().x;
            mMouse.y = (float)event.getPos().y;
        });

        getSignalCleanup().connect([&] { writeConfig(); });

        getWindow()->getSignalKeyUp().connect([&](KeyEvent& event) {
            auto key = event.getCode();
            if (key == KeyEvent::KEY_ESCAPE)
                quit();
            if (key == KeyEvent::KEY_f)
                setFullScreen(!isFullScreen());
        });

        mLoadingContext = gl::env()->createSharedContext(gl::context());

        mToyNames = listToyFiles();
#ifndef CINDER_COCOA_TOUCH
        auto params = createConfigUI({300, 500});
        ADD_ENUM_TO_INT(params.get(), TOY_ID, mToyNames);
        params->addParam("TEST_VALUE.x", &TEST_VALUE.x).group("TEST_VALUE").label("x").step(0.1f);
        params->addParam("TEST_VALUE.y", &TEST_VALUE.y).group("TEST_VALUE").label("y").step(0.1f);
        params->addParam("TEST_VALUE.z", &TEST_VALUE.z).group("TEST_VALUE").label("z").step(0.1f);
        params->addParam("TEST_VALUE.w", &TEST_VALUE.w).group("TEST_VALUE").label("w").step(0.1f);
        params->addParam("TEST_COLOR", &TEST_COLOR);
#endif
        mChannel0 = am::texture2d(TEX0_NAME);
        mChannel1 = am::texture2d(TEX1_NAME);
        mChannel2 = am::texture2d(TEX2_NAME);
        mChannel3 = am::texture2d(TEX3_NAME);

        getSignalUpdate().connect([&] {
            _FPS = getAverageFps();
            // Calculate shader parameters.
            // https://www.shadertoy.com/howto
            vec3 iResolution(vec2(getWindowSize()), 1);
            float iGlobalTime = (float)getElapsedSeconds();
            float iTimeDelta = 1.0f / _FPS;
            float iFrame = (float)getElapsedFrames();
            float iChannelTime0 = (float)getElapsedSeconds();
            float iChannelTime1 = (float)getElapsedSeconds();
            float iChannelTime2 = (float)getElapsedSeconds();
            float iChannelTime3 = (float)getElapsedSeconds();
            vec3 iChannelResolution0 = mChannel0 ? vec3(mChannel0->getSize(), 1) : vec3(1);
            vec3 iChannelResolution1 = mChannel1 ? vec3(mChannel1->getSize(), 1) : vec3(1);
            vec3 iChannelResolution2 = mChannel2 ? vec3(mChannel2->getSize(), 1) : vec3(1);
            vec3 iChannelResolution3 = mChannel3 ? vec3(mChannel3->getSize(), 1) : vec3(1);
            float iSampleRate = 44100;

            time_t now = time(0);
            tm* t = gmtime(&now);
            vec4 iDate(float(t->tm_year + 1900), float(t->tm_mon + 1), float(t->tm_mday),
                       float(t->tm_hour * 3600 + t->tm_min * 60 + t->tm_sec));

            if (mToyID != TOY_ID)
            {
                mWatchConnection.disconnect();
                mWatchConnection = mFileWatcher.watch(
                    getAssetPath(mToyNames[TOY_ID]), [&](const WatchEvent& event) {
                        try
                        {
                            GLSL_ERROR = "";
                            std::string vs = am::str("common/shadertoy.vert");
                            std::string fs = am::str("common/shadertoy.inc") +
                                             loadString(loadAsset(mToyNames[TOY_ID]));

                            mLoadingContext->makeCurrent();
                            auto format = gl::GlslProg::Format().vertex(vs).fragment(fs);
                        // https://github.com/mattdesl/lwjgl-basics/wiki/GLSL-Versions
#if defined(CINDER_GL_ES)
                            format.version(300); // es 3.0
#else
                        format.version(330); // gl 3.3
#endif
                            auto newGlslProg = gl::GlslProg::create(format);
                            dispatchAsync([&, newGlslProg] {
                                mGlslProg = newGlslProg;
                                mToyID = TOY_ID;
                            });
                        }
                        catch (const std::exception& e)
                        {
                            // Uhoh, something went wrong, but it's not fatal.
                            CI_LOG_EXCEPTION("Failed to compile the shader: ", e);
                            GLSL_ERROR = e.what();
                            TOY_ID = mToyID;
                        }
                    });
            }
            if (mGlslProg)
            {
                // Set shader uniforms.
                mGlslProg->uniform("iResolution", iResolution);
                mGlslProg->uniform("iTime", iGlobalTime);
                mGlslProg->uniform("iTimeDelta", iTimeDelta);
                mGlslProg->uniform("iFrame", iFrame);
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
                mGlslProg->uniform("iSampleRate", iSampleRate);

                mGlslProg->uniform("TEST_VALUE", TEST_VALUE);
                mGlslProg->uniform("TEST_COLOR", TEST_COLOR);
            }
        });

        getWindow()->getSignalDraw().connect([&] {
            gl::clear();

            if (!mGlslProg)
                return;

            gl::ScopedTextureBind tex0(mChannel0, 0);
            gl::ScopedTextureBind tex1(mChannel1, 1);
            gl::ScopedTextureBind tex2(mChannel2, 2);
            gl::ScopedTextureBind tex3(mChannel3, 3);
            gl::ScopedGlslProg glsl(mGlslProg);

            gl::drawSolidRect(Rectf(0, (float)getWindowHeight(), (float)getWindowWidth(), 0));

#if 0
            gl::disableAlphaBlending();
            gl::setMatricesWindow( app::getWindowSize() ); 
            gl::drawString(GLSL_ERROR, { 10, 10 });
            gl::disableAlphaBlending();
#endif
        });
    }

  private:
    gl::GlslProgRef mGlslProg;
    gl::ContextRef mLoadingContext;
    vec4 TEST_VALUE;
    ColorA TEST_COLOR;
    //! Texture slots for our shader, based on ShaderToy.
    gl::TextureRef mChannel0;
    gl::TextureRef mChannel1;
    gl::TextureRef mChannel2;
    gl::TextureRef mChannel3;
    //! Our mouse position: xy = current position while mouse down, zw = last click position.
    vec4 mMouse;
    int mToyID = -1;
    vector<string> mToyNames;
    FileWatcher mFileWatcher;
    signals::Connection mWatchConnection;
};

CINDER_APP(ShaderToyApp, RendererGl, [](App::Settings* settings) {
    readConfig();
    settings->setWindowPos(APP_X, APP_Y);
    settings->setWindowSize(APP_WIDTH, APP_HEIGHT);
    settings->setMultiTouchEnabled(false);
})
