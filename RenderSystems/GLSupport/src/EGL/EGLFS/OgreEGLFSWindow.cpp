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
--------------------------------------------------------------------------*/

#include "OgreRoot.h"
#include "OgreException.h"
#include "OgreLogManager.h"
#include "OgreStringConverter.h"
#include "OgreWindowEventUtilities.h"

#include "OgreGLES2Prerequisites.h"
#include "OgreGLES2RenderSystem.h"

#include "OgreEGLFSSupport.h"
#include "OgreEGLFSWindow.h"
#include "OgreEGLFSContext.h"
#include "EGL/eglplatform.h"

//#include <gbm.h>

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <iostream>
#include <algorithm>
#include <climits>

namespace Ogre {
  EGLFSWindow::EGLFSWindow(EGLFSSupport *glsupport)
  : EGLWindow(glsupport)
  //, mParentWindow(glsupport)   todo
  {
    mGLSupport = glsupport;
    mEglDisplay =  mGLSupport->getGLDisplay();
    mNativeDisplay = 0;
  }

  EGLFSWindow::~EGLFSWindow()
  {
    mWindow = 0;
  }


  EGLContext * EGLFSWindow::createEGLContext()
  {
    std::cout << __FUNCTION__ << " Requesting EGL context" << '\n';
    std::cout << "mEglDisplay " << mEglDisplay << '\n';
    std::cout << "mGLSupport "  << mGLSupport << '\n';
    std::cout << "mEglConfig "  << mEglConfig << '\n';
    std::cout << "mEglSurface " << mEglSurface << '\n';

    return new EGLFSContext(mEglDisplay, mGLSupport, mEglConfig, mEglSurface);
  }

  void EGLFSWindow::getLeftAndTopFromNativeWindow( int & left, int & top, uint width, uint height )
  {
  }

  void EGLFSWindow::initNativeCreatedWindow(const NameValuePairList *miscParams)
  {
    if (miscParams)
    {
      NameValuePairList::const_iterator opt;
      NameValuePairList::const_iterator end = miscParams->end();

      mExternalWindow = 0;
      mNativeDisplay = 0;

      if ((opt = miscParams->find("parentWindowHandle")) != end)
      {
        //vector<String>::type tokens = StringUtil::split(opt->second, " :");
        StringVector tokens = StringUtil::split(opt->second, " :");

        if (tokens.size() == 3)
        {
          // deprecated display:screen:xid format
        }
        else
        {
          // xid format
        }
      }
      else if ((opt = miscParams->find("externalWindowHandle")) != end)
      {
        //vector<String>::type tokens = StringUtil::split(opt->second, " :");
        StringVector tokens = StringUtil::split(opt->second, " :");

        LogManager::getSingleton().logMessage(
          "EGLWindow::create: The externalWindowHandle parameter is deprecated.\n"
          "Use the parentWindowHandle or currentGLContext parameter instead.");
          if (tokens.size() == 3)
          {
            // Old display:screen:xid format
            // The old EGL code always created a "parent" window in this case:
          }
          else if (tokens.size() == 4)
          {
            // Old display:screen:xid:visualinfo format
            mExternalWindow = (NativeWindowType)StringConverter::parseUnsignedLong(tokens[2]);
          }
          else
          {
            // xid format
            mExternalWindow = (NativeWindowType)StringConverter::parseUnsignedLong(tokens[0]);
          }
        }

      }

      // Validate externalWindowHandle
      if (mExternalWindow != 0)
      {
        mEglConfig = 0;
        mEglSurface = createSurfaceFromWindow(mEglDisplay, (NativeWindowType)mExternalWindow);
      } else {
        std::cout << "WARNING - mExternalWindow is Null!!!" << '\n';
      }

      mIsTopLevel = true;

    }

    void EGLFSWindow::createNativeWindow( int &left, int &top, uint &width, uint &height, String &title )
    {
      mEglDisplay = mGLSupport->getGLDisplay();
      eglInitialize( mEglDisplay, nullptr, nullptr );

      left = top = 0;

      mWindow  = NULL;//fbCreateWindow(int(fbGetDisplayByIndex(0)), NULL, NULL, width, height);
      mEglSurface = createSurfaceFromWindow(mEglDisplay, mWindow);


      //mEglDisplay = mGLSupport->getGLDisplay();
      //mEglSurface = createSurfaceFromWindow(mEglDisplay, mWindow);
      EGL_CHECK_ERROR

      if (mIsFullScreen)
      {
        switchFullScreen(true);
      }

        WindowEventUtilities::_addRenderWindow(this);
      }


      void EGLFSWindow::reposition( int left, int top )
      {
        if (mClosed || ! mIsTopLevel)
        {
          return;
        }
      }

      void EGLFSWindow::resize(uint width, uint height)
      {
        eglQuerySurface(mEglDisplay, mEglSurface, EGL_WIDTH, (EGLint*)&width);
        eglQuerySurface(mEglDisplay, mEglSurface, EGL_HEIGHT, (EGLint*)&height);
      }

      void EGLFSWindow::windowMovedOrResized()
      {
      }

      void EGLFSWindow::switchFullScreen(bool fullscreen)
      {
      }

      void EGLFSWindow::create(const String& name, uint width, uint height,
        bool fullScreen, const NameValuePairList *miscParams)
        {
          std::cout << __FUNCTION__ << "Creating a new EGLFS window" << '\n';
          String title = name;
          uint samples = 0;
          int gamma;
          short frequency = 0;
          bool vsync = false;
          ::EGLContext eglContext = 0;
          int left = 0;
          int top  = 0;


          mIsFullScreen = fullScreen;

          if (miscParams)
          {
            NameValuePairList::const_iterator opt;
            NameValuePairList::const_iterator end = miscParams->end();

            if ((opt = miscParams->find("currentGLContext")) != end &&
            StringConverter::parseBool(opt->second))
            {
              eglContext = eglGetCurrentContext();
              EGL_CHECK_ERROR
              if (eglContext == (::EGLContext) 0)
              {
                OGRE_EXCEPT(Exception::ERR_RENDERINGAPI_ERROR,
                  "currentGLContext was specified with no current GL context",
                  "EGLWindow::create");
              }
                std::cout << __FUNCTION__ << "Valid EGL context found!" << '\n';
                eglContext = eglGetCurrentContext();
                EGL_CHECK_ERROR
                mEglSurface = eglGetCurrentSurface(EGL_DRAW);
                EGL_CHECK_ERROR
                mIsExternalGLControl = true;
              }

              // Note: Some platforms support AA inside ordinary windows
              if ((opt = miscParams->find("FSAA")) != end)
              {
                samples = StringConverter::parseUnsignedInt(opt->second);
              }

              if ((opt = miscParams->find("displayFrequency")) != end)
              {
                frequency = (short)StringConverter::parseInt(opt->second);
              }

              if ((opt = miscParams->find("vsync")) != end)
              {
                vsync = StringConverter::parseBool(opt->second);
              }

              if ((opt = miscParams->find("gamma")) != end)
              {
                gamma = StringConverter::parseBool(opt->second);
              }

              if ((opt = miscParams->find("left")) != end)
              {
                left = StringConverter::parseInt(opt->second);
              }

              if ((opt = miscParams->find("top")) != end)
              {
                top = StringConverter::parseInt(opt->second);
              }

              if ((opt = miscParams->find("title")) != end)
              {
                title = opt->second;
              }

              if ((opt = miscParams->find("externalGLControl")) != end)
              {
                mIsExternalGLControl = StringConverter::parseBool(opt->second);
              }
            }

            initNativeCreatedWindow(miscParams);

            if (mEglSurface)
            {
              mEglConfig = mGLSupport->getGLConfigFromDrawable (mEglSurface, &width, &height);
            }

            if (!mEglConfig && eglContext)
            {
              mEglConfig = mGLSupport->getGLConfigFromContext(eglContext);

              if (!mEglConfig)
              {
                // This should never happen.
                OGRE_EXCEPT(Exception::ERR_RENDERINGAPI_ERROR,
                  "Unexpected failure to determine a EGLFBConfig",
                  "EGLWindow::create");
                }
              }

              mIsExternal = (mEglSurface != 0);

              std::cout << __FUNCTION__ << "mIsExternal"  << mIsExternal << '\n';

              if (!mEglConfig)
              {
                int minAttribs[] = {
                  EGL_RED_SIZE,       5,
                  EGL_GREEN_SIZE,     6,
                  EGL_BLUE_SIZE,      5,
                  EGL_DEPTH_SIZE,     16,
                  EGL_SAMPLES,        0,
                  EGL_ALPHA_SIZE,     EGL_DONT_CARE,
                  EGL_STENCIL_SIZE,   EGL_DONT_CARE,
                  EGL_SAMPLE_BUFFERS,  0,
                  EGL_NONE
                };

                int maxAttribs[] = {
                  EGL_RED_SIZE,       8,
                  EGL_GREEN_SIZE,     8,
                  EGL_BLUE_SIZE,      8,
                  EGL_DEPTH_SIZE,     24,
                  EGL_ALPHA_SIZE,     8,
                  EGL_STENCIL_SIZE,   8,
                  EGL_SAMPLE_BUFFERS, 1,
                  EGL_SAMPLES, samples,
                  EGL_NONE
                };

                mEglConfig = mGLSupport->selectGLConfig(minAttribs, maxAttribs);
                mHwGamma = false;
              }

              if (!mIsTopLevel)
              {
                mIsFullScreen = false;
                left = top = 0;
              }

              if (mIsFullScreen)
              {
                mGLSupport->switchMode (width, height, frequency);
              }

              //eglBindAPI( EGL_OPENGL_ES_API );

              if (!mIsExternal)
              {
                createNativeWindow(left, top, width, height, title);
              }

              mContext = createEGLContext();
              //mContext->setCurrent();
              EGL_CHECK_ERROR

              if (!mIsExternalGLControl) {
                mContext = createEGLContext();

                ::EGLSurface oldDrawableDraw = eglGetCurrentSurface(EGL_DRAW);
                EGL_CHECK_ERROR
                ::EGLSurface oldDrawableRead = eglGetCurrentSurface(EGL_READ);
                EGL_CHECK_ERROR
                ::EGLContext oldContext  = eglGetCurrentContext();
                EGL_CHECK_ERROR

                int glConfigID;

                mGLSupport->getGLConfigAttrib(mEglConfig, EGL_CONFIG_ID, &glConfigID);
                LogManager::getSingleton().logMessage("EGLWindow::create used FBConfigID = " + StringConverter::toString(glConfigID));
              }

              mName = name;
              mWidth = width;
              mHeight = height;
              mLeft = left;
              mTop = top;
              mActive = true;
              mVisible = true;

              mClosed = false;

              EGL_CHECK_ERROR
              std::cout << __FUNCTION__ << "EGLFS window created!" << '\n';

            }

          }
