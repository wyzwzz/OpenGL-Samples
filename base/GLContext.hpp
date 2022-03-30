#pragma once
#include "logger.hpp"
#include <functional>
#include <memory>
#include <string>
namespace gl
{

enum EventAction
{
    Press = 0x01,
    Release = 0x02,
    Repeat = 0x03,
    Move = 0x04
};

enum MouseButton
{
    Mouse_Left = 0x01,
    Mouse_Right = 0x02
};

enum KeyButton
{
    Key_Unknown,
    Key_0,
    Key_1,
    Key_2,
    Key_3,
    Key_4,
    Key_5,
    Key_6,
    Key_7,
    Key_8,
    Key_9,

    Key_A,
    Key_B,
    Key_C,
    Key_D,
    Key_E,
    Key_F,
    Key_G,
    Key_H,
    Key_I,
    Key_J,
    Key_K,
    Key_L,
    Key_M,
    Key_N,
    Key_O,
    Key_P,
    Key_Q,
    Key_R,
    Key_S,
    Key_T,
    Key_U,
    Key_V,
    Key_W,
    Key_X,
    Key_Y,
    Key_Z,

    Key_Up,
    Key_Down,
    Key_Left,
    Key_Right,

    Key_Esc
};

using MouseEventCallback = std::function<void(void *, MouseButton, EventAction, int, int)>;
using ScrollEventCallback = std::function<void(void *, int, int)>;
using KeyboardEventCallback = std::function<void(void *, KeyButton, EventAction)>;
using FileDropEventCallback = std::function<void(void *, int, const char **)>;
using FramebufferResizeEventCallback = std::function<void(void *, int, int)>;

struct EventListenerTraits
{
    inline static MouseEventCallback MouseEvent;
    inline static ScrollEventCallback ScrollEvent;
    inline static KeyboardEventCallback KeyboardEvent;
    inline static FileDropEventCallback FileDropEvent;
    inline static FramebufferResizeEventCallback FramebufferResizeEvent;
};

inline std::string GetGLErrorStr(GLenum gl_error)
{
    std::string error;
    switch (gl_error)
    {
    case GL_INVALID_ENUM:
        error = "GL_INVALID_ENUM";
        break;
    case GL_INVALID_VALUE:
        error = "GL_INVALID_VALUE";
        break;
    case GL_INVALID_OPERATION:
        error = "GL_INVALID_OPERATION";
        break;
    case GL_STACK_OVERFLOW:
        error = "GL_STACK_OVERFLOW";
        break;
    case GL_STACK_UNDERFLOW:
        error = "GL_STACK_UNDERFLOW";
        break;
    case GL_OUT_OF_MEMORY:
        error = "GL_OUT_OF_MEMORY";
        break;
    case GL_INVALID_FRAMEBUFFER_OPERATION:
        error = "GL_INVALID_FRAMEBUFFER_OPERATION";
        break;
    case GL_INVALID_INDEX:
        error = "GL_INVALID_INDEX";
        break;
    default:
        error = "UNKNOWN_ERROR";
        break;
    }
    return error;
}

inline void PrintGLErrorType(GLenum gl_error)
{
    LOG_ERROR("{}", GetGLErrorStr(gl_error));
}

inline GLenum PrintGLErrorMsg(const char *file, int line)
{
    GLenum error_code;
    while ((error_code = glGetError()) != GL_NO_ERROR)
    {
        std::string error;
        switch (error_code)
        {
        case GL_INVALID_ENUM:
            error = "GL_INVALID_ENUM";
            break;
        case GL_INVALID_VALUE:
            error = "GL_INVALID_VALUE";
            break;
        case GL_INVALID_OPERATION:
            error = "GL_INVALID_OPERATION";
            break;
        case GL_STACK_OVERFLOW:
            error = "GL_STACK_OVERFLOW";
            break;
        case GL_STACK_UNDERFLOW:
            error = "GL_STACK_UNDERFLOW";
            break;
        case GL_OUT_OF_MEMORY:
            error = "GL_OUT_OF_MEMORY";
            break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            error = "GL_INVALID_FRAMEBUFFER_OPERATION";
            break;
        case GL_INVALID_INDEX:
            error = "GL_INVALID_INDEX";
            break;
        }
        LOG_ERROR("{} at line {} in file {}", error_code, line, file);
    }
    return error_code;
}

#ifdef NDEBUG
#define GL_REPORT void(0);
#define GL_ASSERT void(0);
#define GL_EXPR(expr) expr;
#define GL_CHECK void(0);
#else
#define GL_REPORT PrintGLErrorMsg(__FILE__, __LINE__);
#define GL_ASSERT assert(glGetError() == GL_NO_ERROR);

#define GL_EXPR(expr)                                                                                                  \
    do                                                                                                                 \
    {                                                                                                                  \
        GLenum gl_error;                                                                                               \
        int count = 0;                                                                                                 \
        while ((gl_error = glGetError()) != GL_NO_ERROR)                                                               \
        {                                                                                                              \
            LOG_ERROR("GL error {} before call {} at line {} in file {}", GetGLErrorStr(gl_error), #expr, __LINE__,    \
                      __FILE__);                                                                                       \
            count++;                                                                                                   \
            if (count > 10)                                                                                            \
                break;                                                                                                 \
        }                                                                                                              \
        expr;                                                                                                          \
        count = 0;                                                                                                     \
        while ((gl_error = glGetError()) != GL_NO_ERROR)                                                               \
        {                                                                                                              \
            LOG_ERROR("Calling {} caused GL error {} at line {} in file {}", #expr, GetGLErrorStr(gl_error), __LINE__, \
                      __FILE__);                                                                                       \
            if (++count > 10)                                                                                          \
                break;                                                                                                 \
        }                                                                                                              \
    } while (false);

#define GL_CHECK                                                                                                       \
    do                                                                                                                 \
    {                                                                                                                  \
        GLenum gl_error;                                                                                               \
        int count = 0;                                                                                                 \
        while ((gl_error = glGetError()) != GL_NO_ERROR)                                                               \
        {                                                                                                              \
            LOG_ERROR("GL error {} before line {} in file {}", GetGLErrorStr(gl_error), __LINE__, __FILE__);           \
            if (++count > 10)                                                                                          \
                break;                                                                                                 \
        }                                                                                                              \
    } while (0);

#endif

enum GLObjectType : int
{
    Unknown = 0,
    Buffer,
    Texture,
    Sampler,
    Framebuffer,
    Renderbuffer,
    VertexArray
};

template <typename GLContextImpl, typename GLAPILoaderImpl> class GLContext;

template <typename GLContextImpl, typename GLAPILoaderImpl, GLObjectType type> class GLObject final
{
    using Context = GLContext<GLContextImpl, GLAPILoaderImpl>;
    std::shared_ptr<Context> gl_context;
    uint32_t m_object = 0;
    using Self = GLObject<GLContextImpl, GLAPILoaderImpl, type>;
    template <typename, typename> friend class GLContext;

    GLObject(std::shared_ptr<Context> ctx, uint32_t object) : gl_context(std::move(ctx)), m_object(object)
    {
    }

  public:
    GLObject() = default;
    GLObject(const GLObject &) = delete;
    GLObject &operator=(const GLObject &) = delete;

    GLObject(GLObject &&rhs) noexcept : m_object(rhs.m_object), gl_context(std::move(rhs.gl_context))
    {
        rhs.m_object = 0;
    }
    GLObject &operator=(GLObject &&rhs) noexcept
    {
        Release();
        new (this) Self(std::move(rhs));
        return *this;
    }

    static constexpr GLObjectType GetType()
    {
        return type;
    }

    uint32_t GetGLHandle() const
    {
        return m_object;
    }

    bool IsValid() const
    {
        return gl_context && m_object;
    }

    operator uint32_t() const
    {
        return m_object;
    }

    void Release()
    {
        if (IsValid())
        {
            gl_context->DeleteGLObject(*this);
        }
        gl_context = nullptr;
        m_object = 0;
    }

    ~GLObject()
    {
        Release();
    }
};

template <typename GLContextImpl, typename GLAPILoaderImpl>
class GLContext final : public std::enable_shared_from_this<GLContext<GLContextImpl, GLAPILoaderImpl>>,
                        public GLContextImpl,
                        public GLAPILoaderImpl
{
    using Self = GLContext<GLContextImpl, GLAPILoaderImpl>;
    GLContext()
    {
    }

  public:
    template <GLObjectType type> using GLObject_T = GLObject<GLContextImpl, GLAPILoaderImpl, type>;
    using GLBuffer = GLObject_T<GLObjectType::Buffer>;
    using GLTexture = GLObject_T<GLObjectType::Texture>;
    using GLSampler = GLObject_T<GLObjectType::Sampler>;
    using GLFramebuffer = GLObject_T<GLObjectType::Framebuffer>;
    using GLRenderbuffer = GLObject_T<GLObjectType::Renderbuffer>;
    using GLVertexArray = GLObject_T<GLObjectType::VertexArray>;

    static auto New()
    {
        auto self = new Self();
        return std::shared_ptr<Self>(self);
    }

    auto CreateTexture(GLenum textureTarget)
    {
        uint32_t texture = 0;
        GL_EXPR(glCreateTextures(textureTarget, 1, &texture));
        return GLTexture(this->shared_from_this(), texture);
    }
    void DeleteGLObject(GLTexture &object)
    {
        GL_EXPR(glDeleteTextures(1, &object.m_object));
        object.gl_context = nullptr;
        object.m_object = 0;
    }

    auto CreateBuffer()
    {
        uint32_t handle = 0;
        GL_EXPR(glCreateBuffers(1, &handle));
        return GLBuffer(this->shared_from_this(), handle);
    }
    void DeleteGLObject(GLBuffer &object)
    {
        GL_EXPR(glDeleteBuffers(1, &object.m_object));
        object.gl_context = nullptr;
        object.m_object = 0;
    }

    auto CreateFramebuffer()
    {
        uint32_t handle = 0;
        GL_EXPR(glGenFramebuffers(1, &handle));
        return GLFramebuffer(this->shared_from_this(), handle);
    }
    void DeleteGLObject(GLFramebuffer &object)
    {
        GL_EXPR(glDeleteFramebuffers(1, &object.m_object));
        object.gl_context = nullptr;
        object.m_object = 0;
    }

    auto CreateRenderbuffer()
    {
        uint32_t handle = 0;
        GL_EXPR(glCreateRenderbuffers(1, &handle));
        return GLRenderbuffer(this->shared_from_this(), handle);
    }
    void DeleteGLObject(GLRenderbuffer &object)
    {
        GL_EXPR(glDeleteRenderbuffers(1, &object.m_object));
        object.gl_context = nullptr;
        object.m_object = 0;
    }

    auto CreateSampler()
    {
        uint32_t handle = 0;
        GL_EXPR(glCreateSamplers(1, &handle));
        return GLSampler(this->shared_from_this(), handle);
    }
    void DeleteGLObject(GLSampler &object)
    {
        GL_EXPR(glDeleteSamplers(1, &object.m_object));
        object.gl_context = nullptr;
        object.m_object = 0;
    }

    auto CreateVertexArray()
    {
        uint32_t handle = 0;
        GL_EXPR(glCreateVertexArrays(1, &handle));
        return GLVertexArray(this->shared_from_this(), handle);
    }
    void DeleteGLObject(GLVertexArray &object)
    {
        GL_EXPR(glDeleteVertexArrays(1, &object.m_object));
        object.gl_context = nullptr;
        object.m_object = 0;
    }
};

} // namespace gl