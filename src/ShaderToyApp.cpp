#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Log.h"
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

    vector<string> listToyFiles()
    {
        vector<string> files;
        auto assetRoot = getAssetPath("");
        for (auto& p :
            fs::directory_iterator(assetRoot))
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

        auto params = createConfigUI({ 300, 300 });
        gl::enableDepth();

        getWindow()->getSignalKeyUp().connect([&](KeyEvent& event) {
            if (event.getCode() == KeyEvent::KEY_ESCAPE) quit();
        });

        mToyNames = listToyFiles();
        ADD_ENUM_TO_INT(params.get(), TOY_ID, mToyNames);

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

            if (mToyID != TOY_ID)
            {
                try {
                    GLSL_ERROR = "";
                    std::string vs = am::str("common/shadertoy.vert");
                    std::string fs = am::str("common/shadertoy.inc") + am::str(mToyNames[TOY_ID]);

                    mGlslProg = gl::GlslProg::create(gl::GlslProg::Format()
                        .vertex(vs)
                        .fragment(fs)
                        .define("iTime", "iGlobalTime")
                    );
                    mToyID = TOY_ID;
                }
                catch (const std::exception &e) {
                    // Uhoh, something went wrong, but it's not fatal.
                    CI_LOG_EXCEPTION("Failed to compile the shader: ", e);
                    GLSL_ERROR = e.what();
                }
            }
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
    gl::GlslProgRef     mGlslProg;
    //! Texture slots for our shader, based on ShaderToy.
    gl::TextureRef mChannel0;
    gl::TextureRef mChannel1;
    gl::TextureRef mChannel2;
    gl::TextureRef mChannel3;
    //! Our mouse position: xy = current position while mouse down, zw = last click position.
    vec4 mMouse;
    int mToyID = -1;
    vector<string> mToyNames;
};

CINDER_APP(ShaderToyApp, RendererGl, [](App::Settings* settings) {
    readConfig();
    settings->setWindowSize(APP_WIDTH, APP_HEIGHT);
    settings->setMultiTouchEnabled(false);
})
