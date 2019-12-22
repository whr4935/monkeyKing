#ifndef _RENDERER_H
#define _RENDERER_H

#include <EGL/egl.h>
#define EGL_EGLEXT_PROTOTYPES
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <pthread.h>

#include "MessageQueue.h"
#include "vmath.h"
#include <faceDetection/faceDetection.h>
#include <android/log.h>
#include <jni.h>

class NativeSurfaceTexture;

class Renderer : private MessageQueue {
public:
    enum {
        kWhatExit = 100,
        kWhatInitEGLContext,
        kWhatSetDispSurface,
        kWhatSetEncSurface,
        kWhatCreateTextureObject,
        kWhatStart,
        kWhatStop,
        kWhatFrameAvailable,
        kWhatUpdateFacePosition,
    };

    typedef MessageQueue::Message Message;

    class FrameAvailableListener;
    friend class FrameAvailableListener;

    class MyFaceDetectionListener;
    friend class MyFaceDetectionListener;

public:
    Renderer();
    Renderer(int width, int height);
    virtual ~Renderer();

    void setDispSurface(ANativeWindow* surf);
    void setEncSurface(ANativeWindow* surf);
    void setVideoSize(int width, int heigth);
    FaceDetection* createFaceDetection(FaceDetection::PixelFormat pixelFormat=FaceDetection::PIXEL_FORMAT_RGBA, FaceDetection::FrameListener* frameListener=NULL);
    int start();
    int stop();

    FaceDetection::FrameListener* getFrameListener();
    FaceDetection* getFaceDetection() const {return mFaceDetection;}
    NativeSurfaceTexture* getSurfaceTexture() {return mSurfaceTexture;}
    GLuint createTextureObject();
    void updateFacePosition(const std::vector<int>& result);
    void setTex2FaceDetection(bool enable) {mTex2FaceDetection = enable;}
    bool tex2FaceDetection() const {return mTex2FaceDetection;}

private:
    Renderer(const Renderer&);
    Renderer& operator=(const Renderer&);

    void onFrameAvailable(NativeSurfaceTexture *st);

    int prepareLoop();
    static void* threadWrapper(void*);
    int exitLoop();
    int threadLoop();
    void onThreadExit();

    bool mReady;
    ANativeWindow *mDispSurface;
    ANativeWindow *mEncSurface;
    int mWidth;
    int mHeight;
    int mDispWidth;
    int mDispHeight;

    pthread_t mThreadId;

private:
    int initEGLContext();
    int destroyContext();
    GLuint doCreateTextureObject() const;

    int doStart();
    int doStop();
    void doDestroyGL(); 

    int doDraw();

    void prepareDrawdisp();
    void prepareDrawEnc();

    int createProgram();
    GLuint loadShader(GLenum type, const char*shaderSrc) const;
    void printShaderLog(GLuint shader) const;
    void printProgramLog(GLuint shader) const;


    EGLDisplay mDisplay;
    EGLContext mContext;
    EGLConfig mEGLConfig;
    EGLSurface mEGLDispSurface;
    EGLSurface mEGLEncSurface;

    GLuint mTextureObjectName;

    bool mStarted;
    GLuint mVertexShader;
    GLuint mFragmentShader;
    GLuint mProgramObject;

    GLuint mBufferObject;

    GLuint maPositionLoc;
    GLuint maTextureCoordLoc;
    GLuint muMVPMatrixLoc;
    GLuint muTexMatrixLoc;
    GLuint mtTextureLoc;

    vmath::mat4 mMvpMatrix;
    vmath::mat4 mTexMatrix;


    int loadEffectTexture();
    int createEffectProgram();
    int doDrawEffect();

    void convertFacePosition(const int*p, int size, float(&vertices)[4][2], vmath::mat4 &matrix, int &gender);
    int doShowFaceEffect(float vertices[4][2], vmath::mat4& matrix, int gender);

    const std::vector<int>* mpFacePositions;

    GLuint mEffectVertexShader;
    GLuint mEffectFragmentShader;
    GLuint mEffectProgramObject;

    GLuint maEffectPositionLoc;
    GLuint maEffectTextureCoordLoc;
    GLuint muEffectMVPMatrixLoc;
    GLuint muEffectTexMatrixLoc;
    GLuint mtEffectTextureLoc;

    vmath::mat4 mEffectMvpMatrix;
    vmath::mat4 mEffectTexMatrix;
    vmath::mat4 mEffectiveScaleMatrix;

    GLuint mEffectTextureName[2];

    int packCameraBuffer(int64_t timestampUs = 0ll);
    int bltDraw();
    GLuint mFrameBufferObject;
    GLuint mRenderBufferObject;
    GLuint mOffScreenTexture;
    int mTexSize;
    void* mTexBuffer;
    vmath::mat4 mFlipMatrix;

private:
    FaceDetection* mFaceDetection;
    NativeSurfaceTexture* mSurfaceTexture;
    bool mTex2FaceDetection;

public:
    int getCurrentTimestampExternal();

    jobject mJRenderer;
};

///////////////////////////

class Renderer::MyFaceDetectionListener : public FaceDetectionListener
{
public:
    MyFaceDetectionListener(Renderer *renderer)
        : mRenderer(renderer)
    {
    }

    ~MyFaceDetectionListener()
    {
    }

    int setRenderer(Renderer* renderer)
    {
        mRenderer = renderer;
    }

    int onFrameSize(int width, int height)
    {

    }

    int onUpdateFacePosition(const std::vector<int> &result)
    {
        mRenderer->updateFacePosition(result);
    }

private:
    Renderer *mRenderer;
};

#endif
