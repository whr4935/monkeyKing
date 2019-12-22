#include <android/log.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <jni.h>
#include "renderer.h"
#include "rendererFrameAvailableListener.h"

#define LOG_NDEBUG 0
#define LOG_TAG "Renderer"

#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, ##__VA_ARGS__)
#define ALOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, ##__VA_ARGS__)
#define ALOGE(...) \
    __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, ##__VA_ARGS__)

#undef DEBUG
// #define DEBUG

#define USE_FBO  1

namespace {
JavaVM *g_JVM;
jclass g_rendererHelperClazz;
struct {
    jmethodID loadTexture;
}g_rendererHelperFields;

struct fields_t {
    jfieldID context;
    jmethodID getTimestampExternal;
};
fields_t fields;
}

class RendererFrameListener : public FaceDetection::FrameListener {
public:
    void onFrameReleased(void *cookie)
    {
        free(cookie);
    }
};

///////////////////////////////////////////////////
Renderer::Renderer()
    :Renderer(0, 0)
{

}

Renderer::Renderer(int width, int height)
    : mReady(false)
    , mDispSurface(NULL)
    , mEncSurface(NULL)
    , mWidth(width)
    , mHeight(height)
    , mThreadId(-1)
    , mDisplay(EGL_NO_DISPLAY)
    , mContext(EGL_NO_CONTEXT)
    , mEGLConfig(NULL)
    , mEGLDispSurface(NULL)
    , mEGLEncSurface(NULL)
    , mTextureObjectName(0)
    , mStarted(false)
    , mVertexShader(0)
    , mFragmentShader(0)
    , mProgramObject(0)
    , mBufferObject(0)
    , maPositionLoc(0)
    , maTextureCoordLoc(0)
    , muMVPMatrixLoc(0)
    , muTexMatrixLoc(0)
    , mtTextureLoc(0)
    , mMvpMatrix(vmath::mat4::identity())
    , mTexMatrix(vmath::mat4::identity())
    , mpFacePositions(NULL)
    , mEffectVertexShader(0)
    , mEffectFragmentShader(0)
    , mEffectProgramObject(0)
    , maEffectPositionLoc(0)
    , maEffectTextureCoordLoc(0)
    , muEffectMVPMatrixLoc(0)
    , muEffectTexMatrixLoc(0)
    , mtEffectTextureLoc(0)
    , mEffectMvpMatrix(vmath::mat4::identity())
    , mEffectTexMatrix(vmath::mat4::identity())
    , mEffectiveScaleMatrix(vmath::mat4::identity())
    , mFaceDetection(NULL)
    , mSurfaceTexture(NULL)
    , mTex2FaceDetection(false)
{
    mEffectTextureName[0] = 0;
    mEffectTextureName[1] = 0;

    if (prepareLoop() < 0) {
        ALOGI("create render thread failed!");
        return;
    }

    pushMessage(new Message(kWhatInitEGLContext));
}

Renderer::~Renderer()
{
    exitLoop();

    if (mFaceDetection) {
        delete mFaceDetection;
        mFaceDetection = NULL;
    }

    if (mSurfaceTexture) {
        delete mSurfaceTexture;
        mSurfaceTexture = NULL;
    }
}

void Renderer::setDispSurface(ANativeWindow* surf)
{
    postMessage(new Message(kWhatSetDispSurface, int(surf)));

   GLuint texName = createTextureObject();
   mSurfaceTexture = new NativeSurfaceTexture(texName);
   mSurfaceTexture->setFrameAvailableListener(new FrameAvailableListener(this));
}

void Renderer::setEncSurface(ANativeWindow* surf)
{
   postMessage(new Message(kWhatSetEncSurface, int(surf)));
}

void Renderer::setVideoSize(int width, int height)
{
    mWidth = width;
    mHeight = height;
}

extern FaceDetectionListener* findFaceDetectionListener(void *surface);

FaceDetection* Renderer::createFaceDetection(FaceDetection::PixelFormat pixelFormat, FaceDetection::FrameListener* frameListener)
{
    FaceDetection* faceDetection = NULL;

    if (mWidth == 0 || mHeight == 0) {
        ALOGE("video size hasn't set!");
        return NULL;
    }

    faceDetection = new FaceDetection(mWidth, mHeight, pixelFormat, frameListener);
    if (faceDetection == NULL) {
        ALOGE("create FaceDetection failed!");
        return NULL;
    }

    faceDetection->setListener(new MyFaceDetectionListener(this));

    FaceDetectionListener* faceviewListener = findFaceDetectionListener(mDispSurface);
    if (nullptr != faceviewListener) {
        faceDetection->setListener(faceviewListener);
    }

    mFaceDetection = faceDetection;
    return faceDetection;
}

int Renderer::start()
{
    if (mWidth == 0 || mHeight == 0) {
        ALOGE("video size hasn't set!");
        return -1;
    }

    mEffectMvpMatrix =
        vmath::ortho(0, mWidth, -mHeight, 0, 0, 1.0f);

    mEffectiveScaleMatrix = vmath::scale(1.5f, 0.5f, 1.0f);

    mTexSize = mWidth * mHeight * sizeof(GL_UNSIGNED_BYTE) * 4;
    mFlipMatrix = vmath::mat4::identity();
#if USE_FBO
    mFlipMatrix = vmath::mat4(vmath::vec4(1.0f, 0.0f, 0.0f, 0.0f),
            vmath::vec4(0.0f, -1.0f, 0.0f, 0.0f),
            vmath::vec4(0.0f, 0.0f, 1.0f, 0.0f),
            vmath::vec4(0.0f, 0.0f, 0.0f, 1.0f));
#endif


    postMessage(new Message(kWhatStart));

    if (mFaceDetection != NULL) {
        mFaceDetection->start();
    }

    return 0;
}

int Renderer::stop()
{
    postMessage(new Message(kWhatStop));

    if (mFaceDetection != NULL) {
        mFaceDetection->stop();
    }

    return 0;
}

GLuint Renderer::createTextureObject()
{
    int texName = pushMessage(new Message(kWhatCreateTextureObject));

    return texName;
}

void Renderer::updateFacePosition(const std::vector<int>& result)
{
    std::vector<int>*p = new std::vector<int>(result);

    postMessage(new Message(kWhatUpdateFacePosition, (int)p));
}

void Renderer::onFrameAvailable(NativeSurfaceTexture *st)
{
    postMessage(new Message(kWhatFrameAvailable, int(st)));
}

// ------------------------------------------------
int Renderer::prepareLoop()
{
    int err = pthread_create(&mThreadId, NULL, threadWrapper, (void*)this);
    if (err != 0) {
        return -1;
    }

    mReady = true;
    return 0;
}

void* Renderer::threadWrapper(void *arg)
{
    Renderer *r = static_cast<Renderer*>(arg);
    r->threadLoop();

    return (void*)0;
}

int Renderer::exitLoop()
{
    if (mReady) {
        mReady = false;

        postMessage(new Message(kWhatExit));
        pthread_join(mThreadId, (void**)0);
    }
}

int Renderer::threadLoop()
{
    bool bExit = false;
    Message *msg = NULL;

    while (!bExit) {
        msg = getMessage();
        // ALOGI("get message:%d", msg->what);

        switch (msg->what) {
        case kWhatExit:
            bExit = true;
            break;

        case kWhatInitEGLContext:
            initEGLContext();
            break;

        case kWhatSetDispSurface:
            {
                mDispSurface = reinterpret_cast<ANativeWindow*>(msg->ext1);
                mDispWidth = ANativeWindow_getWidth(mDispSurface);
                mDispHeight = ANativeWindow_getHeight(mDispSurface);

                mEGLDispSurface = eglCreateWindowSurface(mDisplay, mEGLConfig, (EGLNativeWindowType)mDispSurface, NULL);
                if (mEGLDispSurface == EGL_NO_SURFACE) {
                    ALOGE("eglCreateWindowSurface failed:%d", eglGetError());
                }

                EGLBoolean ret = eglMakeCurrent(mDisplay, mEGLDispSurface, mEGLDispSurface, mContext);
                if (ret != EGL_TRUE) {
                    ALOGE("eglMakeCurrent failed:%d", ret);
                }
            }
            break;

        case kWhatSetEncSurface:
            mEncSurface = reinterpret_cast<ANativeWindow*>(msg->ext1);
            mEGLEncSurface = eglCreateWindowSurface(mDisplay, mEGLConfig, (EGLNativeWindowType)mEncSurface, NULL);
            if (mEGLEncSurface == EGL_NO_SURFACE) {
                ALOGE("eglCreateWindowSurface failed:%#x", eglGetError());
            }
            break;

        case kWhatCreateTextureObject:
            {
                GLuint texName = doCreateTextureObject();
                mTextureObjectName = texName;
                msg->ret = texName;
            }
            break;

        case kWhatStart:
            {
                doStart();
            }
            break;

        case kWhatStop:
            {
                doStop();
            }
            break;


        case kWhatFrameAvailable:
            {
                NativeSurfaceTexture* st = reinterpret_cast<NativeSurfaceTexture*>(msg->ext1);

                st->updateTexImage();

                GLfloat tmpMatrix[16];
                st->getTransformMatrix(tmpMatrix);
                memcpy((GLfloat*)mTexMatrix, tmpMatrix, 16);
#if !USE_FBO

                if (mEGLEncSurface != EGL_NO_SURFACE) {
                    prepareDrawEnc();
                    doDraw();
                    doDrawEffect();

                    int64_t nsecs = st->getTimestamp();
                    eglPresentationTimeANDROID(mDisplay, mEGLEncSurface, (EGLnsecsANDROID)nsecs);
                    eglSwapBuffers(mDisplay, mEGLEncSurface);
                }

                prepareDrawdisp();
                doDraw();
                doDrawEffect();
                eglSwapBuffers(mDisplay, mEGLDispSurface);

#else

                glBindFramebuffer(GL_FRAMEBUFFER, mFrameBufferObject);
                glViewport(0, 0, mWidth, mHeight);
                glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
                glClear(GL_COLOR_BUFFER_BIT);

                doDraw();
                if (tex2FaceDetection()) {
                    if (mFaceDetection && !mFaceDetection->isBusy()) {
                        // int64_t nsecs = st->getTimestamp();
                        // packCameraBuffer(nsecs/1000ll);

                        int msecs = getCurrentTimestampExternal();
                        packCameraBuffer(msecs*1000ll);
                    }
                }

                // doDrawEffect();

                glBindFramebuffer(GL_FRAMEBUFFER, 0);

                if (mEGLEncSurface != EGL_NO_SURFACE) {
                    prepareDrawEnc();
                    bltDraw();
                    eglSwapBuffers(mDisplay, mEGLEncSurface);
                }

                prepareDrawdisp();
                bltDraw();
                eglSwapBuffers(mDisplay, mEGLDispSurface);
#endif
            }
            break;

        case kWhatUpdateFacePosition:
            {
                const std::vector<int>* p = reinterpret_cast<const std::vector<int>*>(msg->ext1);

                if (mpFacePositions != NULL) {
                    delete mpFacePositions;
                    mpFacePositions = NULL;
                }

                if (p->size() != 0) {
                    mpFacePositions = p;
                } else {
                    delete p;
                }
            }
            break;

        default:
            ALOGE("can't reach here!");
            break;
        }

        if (msg->hasFence()) {
            msg->signal();
        } else {
            delete msg;
        }
    }

exit:
    onThreadExit();

    if (mDisplay != EGL_NO_DISPLAY) {
        destroyContext();
    }

    ALOGI("renderer thread exited!");
    return 0;
}

void Renderer::onThreadExit()
{
    doDestroyGL();

    if (mpFacePositions) {
        delete mpFacePositions;
        mpFacePositions = NULL;
    }
}

// ------------------------------------------------
int Renderer::initEGLContext()
{
    const EGLint attribs[] = {
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_RECORDABLE_ANDROID, 1,
        EGL_NONE};

    EGLint major_version;
    EGLint minor_version;
    EGLConfig config;
    EGLint numConfigs;
    EGLint format;
    EGLint width;
    EGLint height;
    GLfloat ratio;

    ALOGI("Initializing context");

    if ((mDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY)) == EGL_NO_DISPLAY) {
        ALOGE("eglGetDisplay() returned error %d", eglGetError());
        return -1;
    }
    if (!eglInitialize(mDisplay, &major_version, &major_version)) {
        ALOGE("eglInitialize() returned error %d", eglGetError());
        return -1;
    }

    if (!eglChooseConfig(mDisplay, attribs, &config, 1, &numConfigs)) {
        ALOGE("eglChooseConfig() returned error %d", eglGetError());
        return -1;
    }

    mEGLConfig = config;

    int attrib2_list[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE,
    };

    if (!(mContext = eglCreateContext(mDisplay, config, 0, attrib2_list))) {
        ALOGE("eglCreateContext() returned error %d", eglGetError());
        return -1;
    }

    GLint value = -1;
    eglQueryContext(mDisplay, mContext, EGL_CONTEXT_CLIENT_VERSION, &value);
    // ALOGI("EGL create client version:%d", value);

    return 0;
}

int Renderer::destroyContext()
{
    ALOGI("Destroying context");

    eglMakeCurrent(mDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(mDisplay, mContext);
    eglTerminate(mDisplay);

    mDisplay = EGL_NO_DISPLAY;
    mContext = EGL_NO_CONTEXT;
}

GLuint Renderer::doCreateTextureObject() const
{
    GLuint texName = 0;

    glGenTextures(1, &texName);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, texName);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // ALOGI("create GL_TEXTURE_EXTERNAL_OES:%d", texName);
    return texName;
}


int Renderer::doStart()
{
    if (mStarted) {
        ALOGI("renderer thread alrady started!");
        return 0;
    }

    glEnable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    int ret = createProgram();
    if (ret < 0) {
        ALOGE("craete program failed!");
        return -1;
    }

    maPositionLoc = glGetAttribLocation(mProgramObject, "aPosition");
    maTextureCoordLoc = glGetAttribLocation(mProgramObject, "aTextureCoord");
    muMVPMatrixLoc = glGetUniformLocation(mProgramObject, "uMVPMatrix");
    muTexMatrixLoc = glGetUniformLocation(mProgramObject, "uTexMatrix");
    mtTextureLoc = glGetUniformLocation(mProgramObject, "sTexture");

/////////////////////
    ret = loadEffectTexture();
    if (ret < 0) {
        ALOGE("load effect texture failed!");
        return -1;
    }

    ret = createEffectProgram();
    if (ret < 0) {
        ALOGE("create effect program failed!");
        return -1;
    }

    maEffectPositionLoc = glGetAttribLocation(mEffectProgramObject, "aPosition");
    maEffectTextureCoordLoc = glGetAttribLocation(mEffectProgramObject, "aTextureCoord");
    muEffectMVPMatrixLoc = glGetUniformLocation(mEffectProgramObject, "uMVPMatrix");
    muEffectTexMatrixLoc = glGetUniformLocation(mEffectProgramObject, "uTexMatrix");
    mtEffectTextureLoc = glGetUniformLocation(mEffectProgramObject, "sTexture");

/////////////////////
#if !USE_FBO
#define BUFFER_STRIDE 6

    static const GLfloat squareVertices[] = {
        -1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 1.0f,
        1.0f, -1.0f,  1.0f, 1.0f, 1.0f, 1.0f,
        -1.0f,  1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        1.0f,  1.0f,  1.0f, 0.0f, 1.0f, 0.0f,
    };
#else
#define BUFFER_STRIDE 6

    static const GLfloat squareVertices[] = {
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
        1.0f, -1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
        -1.0f,  1.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        1.0f,  1.0f,  1.0f, 1.0f, 1.0f, 0.0f,
    };
#endif

    glGenBuffers(1, &mBufferObject);
    if (mBufferObject == 0) {
        ALOGI("%d create buffer object failed:%#x", __LINE__, glGetError());
        return -1;
    }
    glBindBuffer(GL_ARRAY_BUFFER, mBufferObject);
    glBufferData(GL_ARRAY_BUFFER, sizeof(squareVertices), squareVertices, GL_STATIC_DRAW);

/////////////////////
    glGenRenderbuffers(1, &mRenderBufferObject);
    glBindRenderbuffer(GL_RENDERBUFFER, mRenderBufferObject);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, mWidth, mHeight);
    ret = glGetError();
    if (ret != GL_NO_ERROR) {
        ALOGE("glRenderbufferStorage failed:%#x", ret);
    }

    glGenTextures(1, &mOffScreenTexture);
    glBindTexture(GL_TEXTURE_2D, mOffScreenTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mWidth, mHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    ret = glGetError();
    if (ret != GL_NO_ERROR) {
        ALOGE("glBindTexture failed:%#x", ret);
    }

    glGenFramebuffers(1, &mFrameBufferObject);
    glBindFramebuffer(GL_FRAMEBUFFER, mFrameBufferObject);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mRenderBufferObject);
    if ((ret = glGetError()) != GL_NO_ERROR) {
        ALOGE("glFramebufferRenderbuffer failed:%#x", ret);
    }
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mOffScreenTexture, 0);
    if ((ret = glGetError()) != GL_NO_ERROR) {
        ALOGE("glFramebufferTexture2D failed:%#x", ret);
    }

    ret = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (ret != GL_FRAMEBUFFER_COMPLETE) {
        ALOGE("prepare framebuffer object failed:%#x", ret);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    mStarted = true;
    return 0;
}

int Renderer::doStop()
{
    //noting todo

    return 0;
}

void Renderer::doDestroyGL()
{
    mStarted = false;

    if (mTextureObjectName) {
        glDeleteTextures(1, &mTextureObjectName);
        mTextureObjectName = 0;
#ifdef DEBUG
        int err = glGetError();
        if (err != GL_NO_ERROR)
            ALOGI("%d glDeleteTextures error:%#x", __LINE__, err);
#endif
    }

    if (mVertexShader) {
        glDeleteShader(mVertexShader);
        mVertexShader = 0;
    }

    if (mFragmentShader) {
        glDeleteShader(mFragmentShader);
        mFragmentShader = 0;
    }

    if (mProgramObject) {
        glDeleteProgram(mProgramObject);
        mProgramObject = 0;
    }

    if (mBufferObject) {
        glDeleteBuffers(1, &mBufferObject);
        mBufferObject = 0;
    }

///////////////////////////
    if (mEffectTextureName[0]) {
        glDeleteTextures(2, mEffectTextureName);
        mEffectTextureName[0] = 0;
        mEffectTextureName[1] = 0;
    }

    if (mEffectVertexShader) {
        glDeleteShader(mEffectVertexShader);
        mEffectVertexShader = 0;
    }

    if (mEffectFragmentShader) {
        glDeleteShader(mEffectFragmentShader);
        mEffectFragmentShader = 0;
    }

    if (mEffectProgramObject) {
        glDeleteProgram(mEffectProgramObject);
        mEffectProgramObject = 0;
    }
///////////////////////////
    if (mFrameBufferObject) {
        glDeleteFramebuffers(1, &mFrameBufferObject);
        mFrameBufferObject = 0;
    }

    if (mRenderBufferObject) {
        glDeleteRenderbuffers(1, &mRenderBufferObject);
        mRenderBufferObject = 0;
    }

    if (mOffScreenTexture) {
        glDeleteTextures(1, &mOffScreenTexture);
        mOffScreenTexture = 0;
    }


///////////////////////////
    if (mEGLDispSurface != EGL_NO_SURFACE) {
        eglDestroySurface(mDisplay, mEGLDispSurface);
        mEGLDispSurface = EGL_NO_SURFACE;
    }

    if (mEGLEncSurface != EGL_NO_SURFACE) {
        eglDestroySurface(mDisplay, mEGLEncSurface);
        mEGLEncSurface = EGL_NO_SURFACE;
    }
}

int Renderer::doDraw()
{
    glUseProgram(mProgramObject);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, mTextureObjectName);
#ifdef DEBUG
    GLenum err = glGetError();
    if (err != GL_NO_ERROR)
        ALOGI("%d glBindTexture error:%#x", __LINE__, err);
#endif


    glUniformMatrix4fv(muMVPMatrixLoc, 1, false, (GLfloat*)mTexMatrix);
    glUniformMatrix4fv(muTexMatrixLoc, 1, false, (GLfloat*)mTexMatrix);
    glUniform1i(mtTextureLoc, 0);

    glBindBuffer(GL_ARRAY_BUFFER, mBufferObject);
    glVertexAttribPointer(maPositionLoc, 2, GL_FLOAT, false, sizeof(GL_FLOAT)*BUFFER_STRIDE, 0);
    glVertexAttribPointer(maTextureCoordLoc, 2, GL_FLOAT, false, sizeof(GL_FLOAT)*BUFFER_STRIDE, (const GLvoid*)(sizeof(GL_FLOAT)*2));
    glEnableVertexAttribArray(maPositionLoc);
    glEnableVertexAttribArray(maTextureCoordLoc);
#ifdef DEBUG
    err = glGetError();
    if (err != GL_NO_ERROR)
        ALOGI("%d glEnableVertexAttribArray error:%#x", __LINE__, err);
#endif

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glDisableVertexAttribArray(maPositionLoc);
    glDisableVertexAttribArray(maTextureCoordLoc);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);
    glUseProgram(0);

    return 0;
}

void Renderer::prepareDrawdisp()
{
    glViewport(0, 0, mDispWidth, mDispHeight);
    eglMakeCurrent(mDisplay, mEGLDispSurface, mEGLDispSurface, mContext);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void Renderer::prepareDrawEnc()
{
    glViewport(0, 0, mWidth, mHeight);
    eglMakeCurrent(mDisplay, mEGLEncSurface, mEGLEncSurface, mContext);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

int Renderer::createProgram()
{
   GLchar vShaderStr[] =
    "uniform mat4 uMVPMatrix;\n"
    "uniform mat4 uTexMatrix;\n"
    "attribute vec4 aPosition;\n"
    "attribute vec4 aTextureCoord;\n"
    "varying vec2 vTextureCoord;\n"
    "void main() {\n"
    "    gl_Position = uMVPMatrix * aPosition;\n"
    "    vTextureCoord = (uTexMatrix * aTextureCoord).xy;\n"
    "}\n";

   GLchar fShaderStr[] =
    "#extension GL_OES_EGL_image_external : require\n"
	"precision mediump float;               \n"
    "varying vec2 vTextureCoord;\n"
    "uniform samplerExternalOES sTexture;"
    "void main() {"
    "    gl_FragColor = texture2D(sTexture, vTextureCoord);\n"
    "}\n";

   mVertexShader = loadShader(GL_VERTEX_SHADER, vShaderStr);
   mFragmentShader = loadShader(GL_FRAGMENT_SHADER, fShaderStr);

    mProgramObject = glCreateProgram();
    if (mProgramObject == 0) {
        ALOGE("glCreateProgram failed!");
        return -1;
    }

    glAttachShader(mProgramObject, mVertexShader);
    glAttachShader(mProgramObject, mFragmentShader);

    glLinkProgram(mProgramObject);

    GLint linked;
    glGetProgramiv(mProgramObject, GL_LINK_STATUS, &linked);
    if (!linked) {
        ALOGE("glLinkProgram failed");
        printProgramLog(mProgramObject);
        return -1;
    }

#ifdef DEBUG
    glValidateProgram(mProgramObject);
    GLint valid = GL_FALSE;
    glGetProgramiv(mProgramObject, GL_VALIDATE_STATUS, &valid);
    if (valid != GL_TRUE) {
        printProgramLog(mProgramObject);
    }
#endif

    return 0;
}

GLuint Renderer::loadShader(GLenum type, const char* shaderSrc) const
{
    GLuint shader;
    GLint compiled;

    shader = glCreateShader(type);
    if (shader == 0) {
        ALOGI("glCreateShader failed");
        return 0;
    }

    glShaderSource(shader, 1, &shaderSrc, NULL);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        printShaderLog(shader);
        glDeleteShader(shader);
    }

    return shader;
}

void Renderer::printShaderLog(GLuint shader) const
{
    GLint infoLen = 0;

    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
    if (infoLen > 1) {
        char *infoLog = new char[infoLen];
        glGetShaderInfoLog(shader, infoLen, NULL, infoLog);
        ALOGE("Error compiling shader:\n%s", infoLog);

        delete [] infoLog;
    }
}

void Renderer::printProgramLog(GLuint program) const
{
    GLint infoLen = 0;

    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLen);
    if (infoLen > 1) {
        char *infoLog = new char[infoLen];
        glGetProgramInfoLog(program, infoLen, NULL, infoLog);
        ALOGE("Error compiling shader:\n%s", infoLog);

        delete [] infoLog;
    }
}

int Renderer::loadEffectTexture()
{
    JNIEnv *env = NULL;
    int texName = 0;

   int ret = g_JVM->AttachCurrentThread(&env, NULL);
   if (ret != JNI_OK) {
       ALOGE("jvm attach current thread failed!");
       return -1;
   }

    texName = env->CallStaticIntMethod(g_rendererHelperClazz, g_rendererHelperFields.loadTexture, MyFaceDetectionListener::MALE_INDEX);
    if (texName == 0) {
        ALOGE("%d loadEffectTexture failed:%#x", __LINE__, glGetError());
        return -1;
    }
    mEffectTextureName[MyFaceDetectionListener::MALE_INDEX] = texName;

    texName = 0;
    texName = env->CallStaticIntMethod(g_rendererHelperClazz, g_rendererHelperFields.loadTexture, MyFaceDetectionListener::FEMALE_INDEX);
    if (texName == 0) {
        ALOGE("%d loadEffectTexture failed:%#x", __LINE__, glGetError());
        return -1;
    }
    mEffectTextureName[MyFaceDetectionListener::FEMALE_INDEX] = texName;

    g_JVM->DetachCurrentThread();

    return 0;
}

int Renderer::createEffectProgram()
{
   GLchar vShaderStr[] =
    "uniform mat4 uMVPMatrix;\n"
    "uniform mat4 uTexMatrix;\n"
    "attribute vec4 aPosition;\n"
    "attribute vec4 aTextureCoord;\n"
    "varying vec2 vTextureCoord;\n"
    "void main() {\n"
    "    gl_Position = uMVPMatrix * aPosition;\n"
    "    vTextureCoord = (uTexMatrix * aTextureCoord).xy;\n"
    "}\n";

   GLchar fShaderStr[] =
	"precision mediump float;               \n"
    "varying vec2 vTextureCoord;\n"
    "uniform sampler2D sTexture;"
    "void main() {"
    "    gl_FragColor = texture2D(sTexture, vTextureCoord);\n"
    "}\n";

   mEffectVertexShader = loadShader(GL_VERTEX_SHADER, vShaderStr);
   mEffectFragmentShader = loadShader(GL_FRAGMENT_SHADER, fShaderStr);

    mEffectProgramObject = glCreateProgram();
    if (mEffectProgramObject == 0) {
        ALOGE("glCreateProgram failed!");
        return -1;
    }

    glAttachShader(mEffectProgramObject, mEffectVertexShader);
    glAttachShader(mEffectProgramObject, mEffectFragmentShader);

    glLinkProgram(mEffectProgramObject);

    GLint linked;
    glGetProgramiv(mEffectProgramObject, GL_LINK_STATUS, &linked);
    if (!linked) {
        ALOGE("glLinkProgram failed");
        printProgramLog(mEffectProgramObject);
        return -1;
    }

#ifdef DEBUG
    glValidateProgram(mEffectProgramObject);
    GLint valid = GL_FALSE;
    glGetProgramiv(mEffectProgramObject, GL_VALIDATE_STATUS, &valid);
    if (valid != GL_TRUE) {
        printProgramLog(mEffectProgramObject);
    }
#endif

    return 0;
}

int Renderer::doDrawEffect()
{
    if (mpFacePositions == NULL)
        return 0;


    const int size = MyFaceDetectionListener::RESULT_SIZE;
    auto p = mpFacePositions;
    int count = p->size()/size;

    float vertices[4][2];
    vmath::mat4 matrix;
    int gender = 0;

    for (int i = 0; i != count; ++i) {
        convertFacePosition(&p->at(size*i), size, vertices, matrix, gender);
        doShowFaceEffect(vertices, matrix, gender);
    }

    return 0;
}

void Renderer::convertFacePosition(const int *p, int size, float (&vertices)[4][2], vmath::mat4 &matrix, int& gender)
{
    int x = p[0];
    int y = p[1];
    int width = p[2];
    int height = p[3];

    vertices[0][0] = x;
    vertices[0][1] = -y;
    vertices[1][0] = x+width;
    vertices[1][1] = -y;
    vertices[2][0] = x;
    vertices[2][1] = -(y-height);
    vertices[3][0] = x+width;
    vertices[3][1] = -(y-height);

    matrix = mFlipMatrix * mEffectMvpMatrix *
        vmath::translate(-(x+width/2)/2*1.0f, -(y/2+10)*1.0f, 0.0f) *
        mEffectiveScaleMatrix;

    gender = p[4];

    if (gender < MyFaceDetectionListener::MALE_INDEX) {
        gender = MyFaceDetectionListener::MALE_INDEX;
    } else if (gender > MyFaceDetectionListener::FEMALE_INDEX) {
        gender = MyFaceDetectionListener::FEMALE_INDEX;
    }
}

int Renderer::doShowFaceEffect(float vertices[4][2], vmath::mat4& matrix, int gender)
{
    int err;

    glUseProgram(mEffectProgramObject);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, mEffectTextureName[gender]);
    glUniformMatrix4fv(muEffectMVPMatrixLoc, 1, false, (GLfloat*)matrix);
    glUniformMatrix4fv(muEffectTexMatrixLoc, 1, false, (GLfloat*)mEffectTexMatrix);
    glUniform1i(mtEffectTextureLoc, 1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glVertexAttribPointer(maEffectPositionLoc, 2, GL_FLOAT, false, 2*sizeof(GL_FLOAT), vertices);
#ifdef DEBUG
    err = glGetError();
    if (err != GL_NO_ERROR)
        ALOGI("%d glVertexAttribPointer error:%#x", __LINE__, err);
#endif

    glEnableVertexAttribArray(maEffectPositionLoc);

    glBindBuffer(GL_ARRAY_BUFFER, mBufferObject);
    glEnableVertexAttribArray(maEffectTextureCoordLoc);
    glVertexAttribPointer(maEffectTextureCoordLoc, 2, GL_FLOAT, false, sizeof(GL_FLOAT)*BUFFER_STRIDE, (const GLvoid*)(sizeof(GL_FLOAT)*4));

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glDisableVertexAttribArray(maEffectPositionLoc);
    glDisableVertexAttribArray(maEffectTextureCoordLoc);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);

    return 0;
}

int Renderer::packCameraBuffer(int64_t timestampUs)
{
    struct timespec beg_time, end_time;
    clock_gettime(CLOCK_MONOTONIC, &beg_time);

    int err;
    mTexBuffer = calloc(mTexSize, sizeof(char));
    if (mTexBuffer != NULL) {
        // glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glReadPixels(0, 0, mWidth, mHeight, GL_RGBA, GL_UNSIGNED_BYTE, mTexBuffer);
        if ((err = glGetError()) == GL_NO_ERROR) {
            mFaceDetection->sendFrame((uint8_t*)mTexBuffer, timestampUs, mTexBuffer);
        } else {
            ALOGE("glReadPixels error:0x%#x", err);
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &end_time);
    int64_t cost_time = (end_time.tv_sec - beg_time.tv_sec) * 1000000 +
                        (end_time.tv_nsec - beg_time.tv_nsec) / 1000;

    ALOGI("glReadPixels cost time:%.3f ms", cost_time/1e3);

    return 0;
}

int Renderer::bltDraw()
{
    glUseProgram(mEffectProgramObject);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mOffScreenTexture);
#ifdef DEBUG
    GLenum err = glGetError();
    if (err != GL_NO_ERROR)
        ALOGI("%d glBindTexture error:%#x", __LINE__, err);
#endif


    glUniformMatrix4fv(muMVPMatrixLoc, 1, false, (GLfloat*)vmath::mat4::identity());
    glUniformMatrix4fv(muTexMatrixLoc, 1, false, (GLfloat*)vmath::mat4::identity());
    glUniform1i(mtTextureLoc, 0);

    glBindBuffer(GL_ARRAY_BUFFER, mBufferObject);
    glVertexAttribPointer(maPositionLoc, 2, GL_FLOAT, false, sizeof(GL_FLOAT)*BUFFER_STRIDE, 0);
    glVertexAttribPointer(maTextureCoordLoc, 2, GL_FLOAT, false, sizeof(GL_FLOAT)*BUFFER_STRIDE, (const GLvoid*)(sizeof(GL_FLOAT)*4));
    glEnableVertexAttribArray(maPositionLoc);
    glEnableVertexAttribArray(maTextureCoordLoc);
#ifdef DEBUG
    err = glGetError();
    if (err != GL_NO_ERROR)
        ALOGI("%d glEnableVertexAttribArray error:%#x", __LINE__, err);
#endif

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glDisableVertexAttribArray(maPositionLoc);
    glDisableVertexAttribArray(maTextureCoordLoc);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);
    glUseProgram(0);

    return 0;
}

int Renderer::getCurrentTimestampExternal()
{
    JNIEnv* env = NULL;

    int ret = g_JVM->AttachCurrentThread(&env, NULL);
    if (ret != JNI_OK) {
        ALOGE("JVM attach current thread failed!");
        return -1ll;
    }

    int timestamp = env->CallIntMethod(mJRenderer, fields.getTimestampExternal);

    ret = g_JVM->DetachCurrentThread();
    if (ret != JNI_OK) {
        ALOGE("JVM detach current thread failed!");
    }

    return timestamp;
}

/////////////////////////////////////////////////
FaceDetection::FrameListener* Renderer::getFrameListener()
{
    return new RendererFrameListener;
}


/////////////////////////////////////////////////
static inline void setRenderer(JNIEnv* env, jobject thiz, Renderer* renderer)
{
    env->SetLongField(thiz, fields.context, (jlong)renderer);
}

static inline Renderer* getRenderer(JNIEnv* env, jobject thiz)
{
    return (Renderer*)env->GetLongField(thiz, fields.context);
}


static void Java_com_liantong_renderer_renderer_native_init(JNIEnv* env)
{
    jclass clazz;

    clazz = env->FindClass("com/liantong/renderer/Renderer");
    if (clazz == NULL) {
        ALOGE("find class com/liantong/renderer/Renderer failed!");
        return;
    }

    fields.context = env->GetFieldID(clazz, "mNativeObj", "J");
    fields.getTimestampExternal = env->GetMethodID(clazz, "getTimestampExternal", "()I");
}

static void Java_com_liantong_renderer_renderer_native_setup(JNIEnv* env, jobject thiz, jobject surface)
{
   Renderer* renderer = new Renderer();

   ANativeWindow* disp = ANativeWindow_fromSurface(env, surface);

   renderer->setDispSurface(disp);

   setRenderer(env, thiz, renderer);

   jobject gThiz = (jobject)env->NewGlobalRef(thiz);
   renderer->mJRenderer = gThiz;
}

static jobject Java_com_liantong_renderer_renderer_native_getSurface(JNIEnv* env, jobject thiz)
{
    Renderer* renderer = getRenderer(env, thiz);
    jobject jsurface = NULL;

    jsurface = renderer->getSurfaceTexture()->createJSurface(env);
    if (jsurface == NULL) {
        jclass e = env->FindClass("java/lang/IllegalArgumentException");
        env->ThrowNew(e, "create Java Surface failed!");
    }
    return jsurface;
}

static void Java_com_liantong_renderer_renderer_native_setVideoSize(JNIEnv* env, jobject thiz, jint width, jint height)
{
    Renderer* renderer = getRenderer(env, thiz);

    renderer->setVideoSize(width, height);
}

static void Java_com_liantong_renderer_renderer_native_setTex2FaceDetection(JNIEnv* env, jobject thiz, jboolean enable)
{
    Renderer* renderer = getRenderer(env, thiz);

    renderer->setTex2FaceDetection(enable);
}

static void Java_com_liantong_renderer_renderer_native_createFaceDetection(JNIEnv* env, jobject thiz)
{
    Renderer* renderer = getRenderer(env, thiz);
    if (renderer->getFaceDetection() != NULL)
        return;

    FaceDetection::PixelFormat pixelFormat;
    if (renderer->tex2FaceDetection()) {
        pixelFormat = FaceDetection::PIXEL_FORMAT_RGBA;
    } else {
        pixelFormat = FaceDetection::PIXEL_FORMAT_NV21;
    }
    renderer->createFaceDetection(pixelFormat, renderer->getFrameListener());
}

static void Java_com_liantong_renderer_renderer_native_sendFrame(JNIEnv* env, jobject thiz, jbyteArray data)
{
    if (data == NULL) {
        ALOGE("sendFrame data is NULL");
        return;
    }

    Renderer* renderer = getRenderer(env, thiz);
    if (renderer->tex2FaceDetection())
        return;

    FaceDetection* faceDetection = renderer->getFaceDetection();
    if (faceDetection == NULL)
        return;

    jsize size = env->GetArrayLength(data);
    void* buffer = NULL;
    buffer = calloc(size, sizeof(jbyte));
    if (buffer == NULL) {
        ALOGE("allocate buffer failed, size:%#x", size);
        return;
    }

    env->GetByteArrayRegion(data, 0, size, (jbyte*)buffer);
    faceDetection->sendFrame((uint8_t*)buffer, 0ll, buffer);
}

static void Java_com_liantong_renderer_renderer_native_start(JNIEnv* env, jobject thiz)
{
    Renderer* renderer = getRenderer(env, thiz);

    if (renderer->start() < 0) {
        jclass e = env->FindClass("java/lang/IllegalStateException");
        env->ThrowNew(e, "video size hasn't set!");
    }
}

static void Java_com_liantong_renderer_renderer_native_stop(JNIEnv* env, jobject thiz)
{
    Renderer* renderer = getRenderer(env, thiz);

    renderer->stop();
}

static void Java_com_liantong_renderer_renderer_native_setTrain(JNIEnv* env, jobject thiz, jboolean train, jstring jlabel)
{
    Renderer* renderer = getRenderer(env, thiz);

    FaceDetection* faceDetection = renderer->getFaceDetection();
    if (faceDetection) {
        const char* plabel = NULL;
        std::string label;

        if (jlabel != NULL) {
            plabel = env->GetStringUTFChars(jlabel, NULL);
            label = plabel;
        }

        faceDetection->setTrain(train, label);

        if (jlabel != NULL) {
            env->ReleaseStringUTFChars(jlabel, plabel);
        }
    }
}

static void Java_com_liantong_renderer_renderer_native_finalize(JNIEnv* env, jobject thiz)
{
    Renderer* renderer = getRenderer(env, thiz);

    if (renderer != NULL) {
        env->DeleteGlobalRef(renderer->mJRenderer);

        delete renderer;
        setRenderer(env, thiz, NULL);
    }
}

static JNINativeMethod gMethods[] = {
        {"native_init",
                "()V",
                (void *)Java_com_liantong_renderer_renderer_native_init
        },

        {"native_setup",
                "(Landroid/view/Surface;)V",
                (void *)Java_com_liantong_renderer_renderer_native_setup
        },

        {"native_getSurface",
            "()Landroid/view/Surface;",
            (void*)Java_com_liantong_renderer_renderer_native_getSurface
        },

        {"native_setVideoSize",
            "(II)V",
            (void*)Java_com_liantong_renderer_renderer_native_setVideoSize
        },

        {"native_setTexture2FaceDetection",
            "(Z)V",
            (void*)Java_com_liantong_renderer_renderer_native_setTex2FaceDetection
        },

        {"native_createFaceDetection",
            "()V",
            (void*)Java_com_liantong_renderer_renderer_native_createFaceDetection
        },

        {"native_sendFrame",
            "([B)V",
            (void*)Java_com_liantong_renderer_renderer_native_sendFrame
        },

        {"native_start",
            "()V",
            (void*)Java_com_liantong_renderer_renderer_native_start
        },

        {"native_stop",
            "()V",
            (void*)Java_com_liantong_renderer_renderer_native_stop
        },

        {"native_setTrain",
            "(ZLjava/lang/String;)V",
            (void*)Java_com_liantong_renderer_renderer_native_setTrain
        },

        {"native_finalize",
                "()V",
                (void *)Java_com_liantong_renderer_renderer_native_finalize
        },
};

#define NELEM(a) (sizeof(a)/sizeof(*a))

int register_com_liantong_renderer_renderer(JNIEnv *env)
{
    jclass clazz;

    clazz = env->FindClass("com/liantong/renderer/Renderer");
    if (clazz == NULL) {
        if (env->ExceptionCheck()) {
            env->ExceptionClear();
        }
        return -1;
    }

    if (env->RegisterNatives(clazz, gMethods, NELEM(gMethods)) < 0) {
        return -1;
    }

    return 0;
}

/////////////////////////////////////////////////////////////////////////
static void Java_com_liantong_renderer_rendererHelper_native_init(JNIEnv* env)
{
    jclass clazz;

    clazz = env->FindClass("com/liantong/renderer/RendererHelper");
    if (clazz == NULL) {
        ALOGE("find class com/liantong/renderer/RendererHelper failed!");
        return;
    }

    g_rendererHelperFields.loadTexture = env->GetStaticMethodID(clazz, "loadTexture", "(I)I");
    if (g_rendererHelperFields.loadTexture ==NULL) {
        ALOGE("get staticMethodID loadTexture failed!");
    }

   g_rendererHelperClazz= (jclass)env->NewGlobalRef(clazz);
}

static JNINativeMethod gMethods_RendererHelper[] = {
        {"native_init",
                "()V",
                (void *)Java_com_liantong_renderer_rendererHelper_native_init
        },
};

int register_com_liantong_renderer_rendererHelper(JNIEnv *env)
{
    jclass clazz;

    clazz = env->FindClass("com/liantong/renderer/RendererHelper");
    if (clazz == NULL) {
        return -1;
    }

    if (env->RegisterNatives(clazz, gMethods_RendererHelper, NELEM(gMethods_RendererHelper)) < 0) {
        return -1;
    }

    return 0;
}

jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
    JNIEnv *env = NULL;
    jclass clazz = NULL;
    jint result = -1;

    if (g_JVM != NULL)
        return JNI_VERSION_1_4;

    g_JVM = vm;

    if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
    	__android_log_print(ANDROID_LOG_INFO, LOG_TAG, "GetEnv failed");
        goto exit;
    }

    if (register_com_liantong_renderer_rendererHelper(env) < 0) {
        ALOGE("native registeration RendererHelper failed!");
        goto exit;
    }

    if (register_com_liantong_renderer_renderer(env) < 0) {
        ALOGE("native registeration renderer failed!");
        //ignore this error
        result = JNI_VERSION_1_4;
        goto exit;
    }

    result = JNI_VERSION_1_4;
    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "JNI_OnLoad sucess");

exit:
    return result;
}

static void doDestroyJNI()
{
    JNIEnv *env = NULL;
    int texName = -1;

   int ret = g_JVM->AttachCurrentThread(&env, NULL);
   if (ret != JNI_OK) {
       ALOGE("jvm attach current thread failed!");
       return ;
   }

    env->DeleteGlobalRef(g_rendererHelperClazz);
    g_rendererHelperClazz = NULL;

    g_JVM->DetachCurrentThread();
}
