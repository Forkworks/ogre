/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2008 Renato Araujo Oliveira Filho <renatox@gmail.com>
Copyright (c) 2000-2014 Torus Knot Software Ltd

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
-----------------------------------------------------------------------------
*/

#include "OgreException.h"
#include "OgreLogManager.h"
#include "OgreStringConverter.h"
#include "OgreRoot.h"

#include "OgreGLES2Prerequisites.h"
#include "OgreGLES2RenderSystem.h"

#include "OgreEGLFSSupport.h"
#include "OgreEGLFSWindow.h"
#include "OgreEGLFSContext.h"

namespace Ogre {

    EGLFSSupport::EGLFSSupport(int profile)
    : EGLSupport(profile)
    {
        // A connection that might be shared with the application for GL rendering:
        //mGLDisplay = getGLDisplay();

       // Checks if exists an EGLDisplay, to avoid reinitializing it.
        bool exists = false;
        mGLDisplay = eglGetCurrentDisplay();
        if (mGLDisplay == EGL_NO_DISPLAY) {
            mGLDisplay = (EGLDisplay *) 0;
                // A connection that might be shared with the application for GL rendering:
                    mGLDisplay = getGLDisplay();
        } else {
            exists = true;
        }

       if (exists) {
            return;
          }

        EGLConfig *glConfigs;
        int config, nConfigs = 0;

        glConfigs = chooseGLConfig(NULL, &nConfigs);

        for (config = 0; config < nConfigs; config++)
        {
            int caveat, samples;

            getGLConfigAttrib(glConfigs[config], EGL_CONFIG_CAVEAT, &caveat);

            if (caveat != EGL_SLOW_CONFIG)
            {
                getGLConfigAttrib(glConfigs[config], EGL_SAMPLES, &samples);
                mSampleLevels.push_back(StringConverter::toString(samples));
            }
        }

        free(glConfigs);

        removeDuplicates(mSampleLevels);
    }

    EGLFSSupport::~EGLFSSupport()
    {
        if (mGLDisplay)
        {
            eglTerminate(mGLDisplay);
        }
    }

    RenderWindow* EGLFSSupport::newWindow(const String &name,
                                        unsigned int width, unsigned int height,
                                        bool fullScreen,
                                        const NameValuePairList *miscParams)
    {
        //std::cout << "@@ EGLFSSupport::newWindow" << '\n';
        //std::cout << "name"   << name << '\n';
        //std::cout << "width"  << width << '\n';
        //std::cout << "height" << height << '\n';
        EGLWindow* window = new EGLFSWindow(this);
        window->create(name, width, height, fullScreen, miscParams);

        return window;
    }
}
