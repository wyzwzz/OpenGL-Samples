#define _USE_MATH_DEFINES
#include <cmath>
#include <demo.hpp>
#include <shader_program.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <logger.hpp>
#include <imgui.h>
#include <array>

#include "cuda_helper.hpp"
#include <cuda_gl_interop.h>

#include "hdr_loader.h"
#include "volume_kernel.cuh"

static void createEnvironment(cudaTextureObject_t* env_tex,cudaArray_t* env_tex_data,const std::string& path){
    uint32_t width,height;
    float* data;
    if(!load_hdr_float4(&data,&width,&height,path.c_str())){
        throw std::runtime_error("load env map failed");
    }

    const cudaChannelFormatDesc channel_desc = cudaCreateChannelDesc<float4>();

    CUDA_RUNTIME_API_CALL(cudaMallocArray(env_tex_data,&channel_desc,width,height));

    CUDA_RUNTIME_API_CALL(cudaMemcpyToArray(*env_tex_data,0,0,data,width*height*sizeof(float4),cudaMemcpyDefault));

    cudaResourceDesc res_desc;
    memset(&res_desc, 0, sizeof(res_desc));
    res_desc.resType = cudaResourceTypeArray;
    res_desc.res.array.array = *env_tex_data;

    cudaTextureDesc tex_desc;
    memset(&tex_desc, 0, sizeof(tex_desc));
    tex_desc.addressMode[0]   = cudaAddressModeWrap;
    tex_desc.addressMode[1]   = cudaAddressModeClamp;
    tex_desc.addressMode[2]   = cudaAddressModeWrap;
    tex_desc.filterMode       = cudaFilterModeLinear;
    tex_desc.readMode         = cudaReadModeElementType;
    tex_desc.normalizedCoords = 1;

    CUDA_RUNTIME_API_CALL(cudaCreateTextureObject(env_tex,&res_desc,&tex_desc,nullptr));

}
struct Volume{
    std::vector<uint8_t> buffer;
    int width;
    int height;
    int depth;
};
void loadVolume(Volume& volume,const std::string& path){
    std::ifstream in(path,std::ios::binary);
    if(!in.is_open()){
        throw std::runtime_error("failed to load volume");
    }
    in.seekg(0,std::ios::end);
    size_t file_size = in.tellg();
    volume.buffer.resize(file_size);
    in.seekg(0,std::ios::beg);
    in.read(reinterpret_cast<char*>(volume.buffer.data()),file_size);
    uint8_t max_v = 0;
    for(auto x:volume.buffer)
        max_v = std::max(max_v,x);
    std::cout<<"max_v for volume is: "<<(int)max_v<<std::endl;
}
static void createVolume(cudaTextureObject_t* volume_tex,cudaArray_t* volume_tex_data,const Volume& volume){
    cudaChannelFormatDesc channelDesc = cudaCreateChannelDesc<uint8_t>();
    cudaMalloc3DArray(volume_tex_data, &channelDesc,
                      {(size_t)volume.width,(size_t)volume.height,(size_t)volume.depth});
    cudaMemcpy3DParms m;
    memset(&m,0,sizeof(m));
    m.srcPtr = cudaPitchedPtr{const_cast<uint8_t*>(volume.buffer.data()),
                              volume.width * sizeof(uint8_t),
                              (size_t)volume.width,(size_t)volume.height};
    m.dstArray = *volume_tex_data;
    m.kind = cudaMemcpyHostToDevice;
    m.extent = {(size_t)volume.width,(size_t)volume.height,(size_t)volume.depth};

    CUDA_RUNTIME_API_CALL(cudaMemcpy3D(&m));
    cudaResourceDesc texRes;
    memset(&texRes, 0, sizeof(cudaResourceDesc));
    texRes.resType = cudaResourceTypeArray;
    texRes.res.array.array = *volume_tex_data;
    cudaTextureDesc texDesc;
    memset(&texDesc, 0, sizeof(cudaTextureDesc));
    texDesc.normalizedCoords = true;           // access with normalized texture coordinates
    texDesc.filterMode = cudaFilterModeLinear; // linear interpolation
    texDesc.borderColor[0] = 0.f;
    texDesc.borderColor[1] = 0.f;
    texDesc.borderColor[2] = 0.f;
    texDesc.borderColor[3] = 0.f;
    texDesc.addressMode[0] = cudaAddressModeBorder;
    texDesc.addressMode[1] = cudaAddressModeBorder;
    texDesc.addressMode[2] = cudaAddressModeBorder;
    texDesc.readMode = cudaReadModeNormalizedFloat;
    CUDA_RUNTIME_API_CALL(cudaCreateTextureObject(volume_tex, &texRes, &texDesc, NULL) );
}
static void createVolumeTransferFunction(cudaTextureObject_t* tf_tex,cudaArray_t* tf_tex_data,float4* tf,int size){
    const cudaChannelFormatDesc channel_desc = cudaCreateChannelDesc<float4>();

    CUDA_RUNTIME_API_CALL(cudaMallocArray(tf_tex_data,&channel_desc,size));

    CUDA_RUNTIME_API_CALL(cudaMemcpyToArray(*tf_tex_data,0,0,tf,size*sizeof(float4),cudaMemcpyDefault));

    cudaResourceDesc res_desc;
    memset(&res_desc, 0, sizeof(res_desc));
    res_desc.resType = cudaResourceTypeArray;
    res_desc.res.array.array = *tf_tex_data;

    cudaTextureDesc tex_desc;
    memset(&tex_desc, 0, sizeof(tex_desc));
    tex_desc.addressMode[0]   = cudaAddressModeClamp;
    tex_desc.filterMode       = cudaFilterModeLinear;
    tex_desc.readMode         = cudaReadModeElementType;
    tex_desc.normalizedCoords = 1;

    CUDA_RUNTIME_API_CALL(cudaCreateTextureObject(tf_tex,&res_desc,&tex_desc,nullptr));

}
class VBTApplication final :public  Demo{
  public:
    VBTApplication() = default;
    void initResource() override;
    void render_frame() override ;
    void render_imgui() override;
  private:
    void process_input() override;
    void initCUDAResource();
    void updateCamera();
  private:
    struct{
        GL::GLFramebuffer fbo;
        GL::GLRenderbuffer rbo;
        GL::GLTexture color_attachment;
        GL::GLBuffer color_pbo;
        int width = 960;
        int height = 540;
    }frame;
    struct{
        cudaArray_t env_array;
        cudaArray_t volume_array;
        cudaArray_t volume_tf_array;
        cudaGraphicsResource_t cuda_display_buffer = nullptr;
        VolumeKernelParams kernel_params;

    }cuda_resource;
    Volume volume;
};
static void initCUDA()
{
    int cuda_devices[1];
    unsigned int num_cuda_devices;
    CUDA_RUNTIME_API_CALL(cudaGLGetDevices(&num_cuda_devices, cuda_devices, 1, cudaGLDeviceListAll));
    if (num_cuda_devices == 0){
        fprintf(stderr, "Could not determine CUDA device for current OpenGL context\n.");
        exit(EXIT_FAILURE);
    }
    CUDA_RUNTIME_API_CALL(cudaSetDevice(cuda_devices[0]));
}
void VBTApplication::initCUDAResource()
{
    cuda_resource.kernel_params.resolution = make_uint2(frame.width,frame.height);
    cuda_resource.kernel_params.exposure_scale = 1.f;
    cuda_resource.kernel_params.iteration = 0;
    cuda_resource.kernel_params.max_interactions = 1024;
    cuda_resource.kernel_params.max_extinction = 100.f;
    const std::string ENV_MAP_PATH = "C:/Users/wyz/projects/OpenGL-Samples/data/textures/IBL/hdr/newport_loft.hdr";
    createEnvironment(&cuda_resource.kernel_params.env_tex,&cuda_resource.env_array,ENV_MAP_PATH);

    const std::string VOLUME_PATH = "E:/Volume/foot_256_256_256_uint8.raw";
    volume.width = 256;
    volume.height = 256;
    volume.depth = 256;
    loadVolume(volume,VOLUME_PATH);
    createVolume(&cuda_resource.kernel_params.volume_tex,&cuda_resource.volume_array,volume);

    std::vector<float4> tf(256, make_float4(0.f,0.f,0.f,0.f));
    for(int i = 127; i < 256;i ++){
        float x = 1.f * i / 255.f;
        tf[i] = make_float4(x,x,x,x);
    }
    createVolumeTransferFunction(&cuda_resource.kernel_params.volume_tf_tex,&cuda_resource.volume_tf_array,tf.data(),256);

    //register gl texture to cuda buffer
    {
        //display
        CUDA_RUNTIME_API_CALL(cudaGraphicsGLRegisterBuffer(&cuda_resource.cuda_display_buffer,frame.color_pbo,cudaGraphicsRegisterFlagsWriteDiscard));
        //accumulate
        CUDA_RUNTIME_API_CALL(cudaMalloc(&cuda_resource.kernel_params.accum_buffer,frame.width*frame.height*sizeof(float3)));
    }
}
void VBTApplication::initResource()
{
    camera = std::make_unique<control::FPSCamera>(glm::vec3{0.f,0.f,3.f});

    frame.fbo = gl->CreateFramebuffer();
    frame.rbo = gl->CreateRenderbuffer();
    glBindFramebuffer(GL_FRAMEBUFFER,frame.fbo);
    glBindRenderbuffer(GL_RENDERBUFFER,frame.rbo);
    GL_EXPR(glRenderbufferStorage(GL_RENDERBUFFER,GL_DEPTH24_STENCIL8,frame.width,frame.height));
    GL_EXPR(glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_STENCIL_ATTACHMENT,GL_RENDERBUFFER,frame.rbo));
    frame.color_attachment = gl->CreateTexture(GL_TEXTURE_2D);
    GL_EXPR(glTextureStorage2D(frame.color_attachment,1,GL_RGBA8,frame.width,frame.height));
    GL_EXPR(glFramebufferTexture(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,frame.color_attachment,0));
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE){
        throw std::runtime_error("framebuffer is not complete!");
    }
    glBindFramebuffer(GL_FRAMEBUFFER,0);
    frame.color_pbo = gl->CreateBuffer();
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER,frame.color_pbo);
    glBufferData(GL_PIXEL_UNPACK_BUFFER,frame.width*frame.height*4,NULL,GL_DYNAMIC_COPY);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER,0);

    initCUDA();
    initCUDAResource();
}
void VBTApplication::updateCamera()
{
    static bool first = true;
    static vec3f last_cam_pos;
    static vec3f last_cam_dir;
    static vec3f last_cam_right;
    static vec3f last_cam_up;
    static float last_camera_zoom;
    auto cam_pos = camera->getCameraPos();
    auto cam_dir = normalize(camera->getCameraLookAt() - camera->getCameraPos());
    auto cam_right = camera->getCameraRight();
    auto cam_up = camera->getCameraUp();
    auto cam_zoom = camera->getZoom();

    if(first || cam_pos != last_cam_pos || cam_dir != last_cam_dir || cam_right != last_cam_right
        || cam_up != last_cam_up || cam_zoom != last_camera_zoom){
        first = false;
        cuda_resource.kernel_params.iteration = 0;
        cuda_resource.kernel_params.cam_pos = make_float3(cam_pos.x,cam_pos.y,cam_pos.z);
        cuda_resource.kernel_params.cam_dir = make_float3(cam_dir.x,cam_dir.y,cam_dir.z);
        cuda_resource.kernel_params.cam_right = make_float3(cam_right.x,cam_right.y,cam_right.z);
        cuda_resource.kernel_params.cam_up = make_float3(cam_up.x,cam_up.y,cam_up.z);
        cuda_resource.kernel_params.cam_focal = float(1.0 / tan(cam_zoom / 2.0 * (2.0 * M_PI / 360.0)));
    }
    last_cam_pos = cam_pos;
    last_cam_dir = cam_dir;
    last_cam_right = cam_right;
    last_cam_up = cam_up;
    last_camera_zoom = cam_zoom;
}
void VBTApplication::render_frame()
{
    glClearColor(0.0,0.0,0.0,0.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    updateCamera();

    CUDA_RUNTIME_API_CALL(cudaGraphicsMapResources(1,&cuda_resource.cuda_display_buffer,0));
    void *p;
    size_t size_p;
    CUDA_RUNTIME_API_CALL(cudaGraphicsResourceGetMappedPointer(&p,&size_p,cuda_resource.cuda_display_buffer));
    cuda_resource.kernel_params.display_buffer = reinterpret_cast<uint32_t*>(p);

    dim3 threads_per_block(16, 16);
    dim3 num_blocks((frame.width + 15) / 16, (frame.height + 15) / 16);
    void *params[] = { &cuda_resource.kernel_params };
    CUDA_RUNTIME_API_CALL(cudaLaunchKernel(
        (const void*)(volume_rt_kernel),
        num_blocks,
        threads_per_block,
        params
        ));
    ++cuda_resource.kernel_params.iteration;

    CUDA_RUNTIME_API_CALL(cudaGraphicsUnmapResources(1,&cuda_resource.cuda_display_buffer));

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER,frame.color_pbo);
    glBindTexture(GL_TEXTURE_2D,frame.color_attachment);
    glTexSubImage2D(GL_TEXTURE_2D,0,0,0,frame.width,frame.height,GL_BGRA,GL_UNSIGNED_BYTE,NULL);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER,0);
    glFinish();
    GL_CHECK


}
void VBTApplication::render_imgui()
{
    ImGui::ShowDemoWindow();
    auto& io = ImGui::GetIO();

    ImGui::Begin("Render");

    ImGui::Image((void*)(intptr_t)(frame.color_attachment.GetGLHandle()),ImVec2(frame.width,frame.height));

    ImGui::End();

    ImGui::Begin("Params");
    ImGui::Text("Volumetric Path Tracer");
    ImGui::Text("FPS: %f",ImGui::GetIO().Framerate);
    ImGui::Text("Iteration: %d",cuda_resource.kernel_params.iteration);
    ImGui::Text("Camera Pos: %.2f %.2f %.2f",camera->getCameraPos().x,camera->getCameraPos().y,camera->getCameraPos().z);
    if (ImGui::BeginTabBar("##TabBar")){
        if (ImGui::BeginTabItem("Canvas")){
            static ImVector<ImVec2> points;
            static ImVec2 scrolling(0.0f, 0.0f);
            static bool opt_enable_grid = true;
            static bool opt_enable_context_menu = true;
            static bool adding_line = false;

            ImGui::Checkbox("Enable grid", &opt_enable_grid);
            ImGui::Checkbox("Enable context menu", &opt_enable_context_menu);
            ImGui::Text("Mouse Left: drag to add lines,\nMouse Right: drag to scroll, click for context menu.");

            // Typically you would use a BeginChild()/EndChild() pair to benefit from a clipping region + own scrolling.
            // Here we demonstrate that this can be replaced by simple offsetting + custom drawing + PushClipRect/PopClipRect() calls.
            // To use a child window instead we could use, e.g:
            //      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));      // Disable padding
            //      ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(50, 50, 50, 255));  // Set a background color
            //      ImGui::BeginChild("canvas", ImVec2(0.0f, 0.0f), true, ImGuiWindowFlags_NoMove);
            //      ImGui::PopStyleColor();
            //      ImGui::PopStyleVar();
            //      [...]
            //      ImGui::EndChild();

            // Using InvisibleButton() as a convenience 1) it will advance the layout cursor and 2) allows us to use IsItemHovered()/IsItemActive()
            ImVec2 canvas_p0 = ImGui::GetCursorScreenPos();      // ImDrawList API uses screen coordinates!
            ImVec2 canvas_sz = ImGui::GetContentRegionAvail();   // Resize canvas to what's available
            if (canvas_sz.x < 50.0f) canvas_sz.x = 50.0f;
            if (canvas_sz.y < 50.0f) canvas_sz.y = 50.0f;
            ImVec2 canvas_p1 = ImVec2(canvas_p0.x + canvas_sz.x, canvas_p0.y + canvas_sz.y);

            // Draw border and background color
            ImGuiIO& io = ImGui::GetIO();
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            draw_list->AddRectFilled(canvas_p0, canvas_p1, IM_COL32(50, 50, 50, 255));
            draw_list->AddRect(canvas_p0, canvas_p1, IM_COL32(255, 255, 255, 255));

            // This will catch our interactions
            ImGui::InvisibleButton("canvas", canvas_sz, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
            const bool is_hovered = ImGui::IsItemHovered(); // Hovered
            const bool is_active = ImGui::IsItemActive();   // Held
            const ImVec2 origin(canvas_p0.x + scrolling.x, canvas_p0.y + scrolling.y); // Lock scrolled origin
            const ImVec2 mouse_pos_in_canvas(io.MousePos.x - origin.x, io.MousePos.y - origin.y);

            // Add first and second point
            if (is_hovered && !adding_line && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                points.push_back(mouse_pos_in_canvas);
                points.push_back(mouse_pos_in_canvas);
                adding_line = true;
            }
            if (adding_line)
            {
                points.back() = mouse_pos_in_canvas;
                if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
                    adding_line = false;
            }

            // Pan (we use a zero mouse threshold when there's no context menu)
            // You may decide to make that threshold dynamic based on whether the mouse is hovering something etc.
            const float mouse_threshold_for_pan = opt_enable_context_menu ? -1.0f : 0.0f;
            if (is_active && ImGui::IsMouseDragging(ImGuiMouseButton_Right, mouse_threshold_for_pan))
            {
                scrolling.x += io.MouseDelta.x;
                scrolling.y += io.MouseDelta.y;
            }

            // Context menu (under default mouse threshold)
            ImVec2 drag_delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Right);
            if (opt_enable_context_menu && ImGui::IsMouseReleased(ImGuiMouseButton_Right) && drag_delta.x == 0.0f && drag_delta.y == 0.0f)
                ImGui::OpenPopupOnItemClick("context");
            if (ImGui::BeginPopup("context"))
            {
                if (adding_line)
                    points.resize(points.size() - 2);
                adding_line = false;
                if (ImGui::MenuItem("Remove one", NULL, false, points.Size > 0)) { points.resize(points.size() - 2); }
                if (ImGui::MenuItem("Remove all", NULL, false, points.Size > 0)) { points.clear(); }
                ImGui::EndPopup();
            }

            // Draw grid + all lines in the canvas
            draw_list->PushClipRect(canvas_p0, canvas_p1, true);
            if (opt_enable_grid)
            {
                const float GRID_STEP = 64.0f;
                for (float x = fmodf(scrolling.x, GRID_STEP); x < canvas_sz.x; x += GRID_STEP)
                    draw_list->AddLine(ImVec2(canvas_p0.x + x, canvas_p0.y), ImVec2(canvas_p0.x + x, canvas_p1.y), IM_COL32(200, 200, 200, 40));
                for (float y = fmodf(scrolling.y, GRID_STEP); y < canvas_sz.y; y += GRID_STEP)
                    draw_list->AddLine(ImVec2(canvas_p0.x, canvas_p0.y + y), ImVec2(canvas_p1.x, canvas_p0.y + y), IM_COL32(200, 200, 200, 40));
            }
            for (int n = 0; n < points.Size; n += 2)
                draw_list->AddLine(ImVec2(origin.x + points[n].x, origin.y + points[n].y), ImVec2(origin.x + points[n + 1].x, origin.y + points[n + 1].y), IM_COL32(255, 255, 0, 255), 2.0f);
            draw_list->PopClipRect();

            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::End();


}
void VBTApplication::process_input()
{
    static auto t = glfwGetTime();
    auto cur_t = glfwGetTime();
    auto delta_t = cur_t - t;
    t = cur_t;
    if(glfwGetKey(gl->GetWindow(),GLFW_KEY_W))
        camera->processKeyEvent(control::CameraDefinedKey::Forward,delta_t);
    if(glfwGetKey(gl->GetWindow(),GLFW_KEY_S))
        camera->processKeyEvent(control::CameraDefinedKey::Backward,delta_t);
    if(glfwGetKey(gl->GetWindow(),GLFW_KEY_A))
        camera->processKeyEvent(control::CameraDefinedKey::Left,delta_t);
    if(glfwGetKey(gl->GetWindow(),GLFW_KEY_D))
        camera->processKeyEvent(control::CameraDefinedKey::Right,delta_t);
    if(glfwGetKey(gl->GetWindow(),GLFW_KEY_Q))
        camera->processKeyEvent(control::CameraDefinedKey::Up,delta_t);
    if(glfwGetKey(gl->GetWindow(),GLFW_KEY_E))
        camera->processKeyEvent(control::CameraDefinedKey::Bottom,delta_t);
}


int main(){
    try
    {
        VBTApplication app;
        app.run();
    }
    catch (const std::exception& err)
    {
        std::cout<<err.what()<<std::endl;
    }
    return 0;
}