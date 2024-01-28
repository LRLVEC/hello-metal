#include <common.h>

#define GLFW_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>
#include <Metal/MTLResource.hpp>

#include <QuartzCore/QuartzCore.hpp>

#include <AppKit/NSWindow.hpp>
#include <AppKit/NSView.hpp>
#include <MetalKit/MTKView.hpp>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_metal.h>

CA::MetalLayer* swapchain = nullptr;

static void quit(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

static void resize(GLFWwindow* window, int width, int height)
{
    swapchain->setDrawableSize(CGSize{ (double)width, (double)height });
}

static void mouse(GLFWwindow* window, double x, double y)
{
    // should call imgui callback here
    printf("%f %f\n", x, y);
}

MTL::Library* create_shader(MTL::Device* device)
{
    const char* shaderSrc = R"(
        #include <metal_stdlib>
        using namespace metal;

        struct v2f
        {
            float4 position [[position]];
        };

        kernel void update_vert(constant float &size [[ buffer(0) ]],
                                device const float2* buffer_in [[ buffer(1) ]],
                                device float2* buffer_out [[ buffer(2) ]],
                                uint tid [[thread_index_in_threadgroup]],
                                uint3 bid [[threadgroup_position_in_grid]])
        {
            buffer_out[tid] = size * buffer_in[tid];
        }

        v2f vertex vertexMain(uint vertexId [[vertex_id]], device const float2* positions [[buffer(0)]])
        {
            v2f o;
            o.position = float4( positions[ vertexId ], 0.0, 1.0 );
            return o;
        }

        half4 fragment fragmentMain(v2f in [[stage_in]])
        {
            return half4(0.0, 1.0, 0.0, 1.0);
        }
    )";

    NS::Error* error = nullptr;
    MTL::Library* library = device->newLibrary(NS::String::string(shaderSrc, NS::StringEncoding::UTF8StringEncoding), nullptr, &error);
    if (!library)
    {
        printf("%s", error->localizedDescription()->utf8String());
        return nullptr;
    }
    return library;
}

MTL::RenderPipelineState* create_render_pipeline(MTL::Device* device, MTL::Library* library)
{
    MTL::Function* vertex_fn = library->newFunction(NS::String::string("vertexMain", NS::StringEncoding::UTF8StringEncoding));
    MTL::Function* frag_fn = library->newFunction(NS::String::string("fragmentMain", NS::StringEncoding::UTF8StringEncoding));

    MTL::RenderPipelineDescriptor* desc = MTL::RenderPipelineDescriptor::alloc()->init();
    desc->setVertexFunction(vertex_fn);
    desc->setFragmentFunction(frag_fn);
    desc->colorAttachments()->object(0)->setPixelFormat(MTL::PixelFormat::PixelFormatBGRA8Unorm_sRGB);

    NS::Error* error = nullptr;
    MTL::RenderPipelineState* render_pipeline = device->newRenderPipelineState(desc, &error);
    vertex_fn->release();
    frag_fn->release();
    desc->release();
    if (!render_pipeline)
    {
        printf("%s", error->localizedDescription()->utf8String());
        return nullptr;
    }
    return render_pipeline;
}

MTL::ComputePipelineState* create_compute_pipeline(MTL::Device* device, MTL::Library* library)
{
    NS::Error* error = nullptr;

    MTL::Function* update_fn = library->newFunction(NS::String::string("update_vert", NS::UTF8StringEncoding));
    MTL::ComputePipelineState* compute_pipeline = device->newComputePipelineState(update_fn, &error);
    update_fn->release();
    if (!compute_pipeline)
    {
        printf("%s", error->localizedDescription()->utf8String());
        return nullptr;
    }
    return compute_pipeline;
}

MTL::Buffer* create_buffer(MTL::Device* device)
{
    vec2 positions[] =
    {
        { -0.8f,  -0.8f },
        {  0.0f,   0.8f },
        {  0.8f, -0.8f }
    };
    MTL::Buffer* vertex_pos_buffer = device->newBuffer(sizeof(positions), MTL::ResourceStorageModeManaged);
    printf("%ld\n", vertex_pos_buffer->length());
    memcpy(vertex_pos_buffer->contents(), positions, sizeof(positions));
    vertex_pos_buffer->didModifyRange(NS::Range::Make(0, vertex_pos_buffer->length()));
    return vertex_pos_buffer;
}

int main(void)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls

    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(640, 480, "GLFW Metal", NULL, NULL);

    MTL::Device* device = MTL::CreateSystemDefaultDevice();
    MTL::CommandQueue* queue = device->newCommandQueue();
    swapchain = (CA::MetalLayer*)glfwGetCocoaMetalLayer(window);
    swapchain->setDevice(device);
    // swapchain->opaque = true;

    ImGui_ImplGlfw_InitForOther(window, true);
    ImGui_ImplMetal_Init(device);

    glfwSetKeyCallback(window, quit);
    glfwSetFramebufferSizeCallback(window, resize);
    // glfwSetCursorPosCallback(window, mouse);
    MTL::ClearColor color{ 1, 0, 0, 1 };

    // Setup style
    ImGui::StyleColorsDark();

    MTL::Library* library = create_shader(device);
    MTL::RenderPipelineState* render_pipeline = create_render_pipeline(device, library);
    MTL::ComputePipelineState* compute_pipeline = create_compute_pipeline(device, library);
    library->release();

    MTL::Buffer* vertex_pos_buffer = create_buffer(device);
    MTL::Buffer* vertex_pos_buffer_render = create_buffer(device);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        color.red = (color.red > 1.0) ? 0 : color.red + 0.01;
        CA::MetalDrawable* surface = swapchain->nextDrawable();
        MTL::RenderPassDescriptor* pass = MTL::RenderPassDescriptor::alloc()->init();
        pass->colorAttachments()->object(0)->setClearColor(color);
        pass->colorAttachments()->object(0)->setLoadAction(MTL::LoadActionClear);
        pass->colorAttachments()->object(0)->setStoreAction(MTL::StoreActionStore);
        pass->colorAttachments()->object(0)->setTexture(surface->texture());

        MTL::CommandBuffer* cmd_buffer = queue->commandBuffer();

        // compute
        // NS::UInteger threadGroupSize = compute_pipeline->maxTotalThreadsPerThreadgroup();
        MTL::ComputeCommandEncoder* compute_encoder = cmd_buffer->computeCommandEncoder();
        MTL::Size gridSize = MTL::Size(1, 1, 1);
        MTL::Size blockSize(3, 1, 1);
        float s = color.red;
        compute_encoder->setComputePipelineState(compute_pipeline);
        compute_encoder->setBytes(&s, sizeof(float), 0);
        compute_encoder->setBuffer(vertex_pos_buffer, 0, 1);
        compute_encoder->setBuffer(vertex_pos_buffer_render, 0, 2);
        compute_encoder->dispatchThreadgroups(gridSize, blockSize); // grid size, block size
        compute_encoder->endEncoding();

        // render
        MTL::RenderCommandEncoder* render_encoder = cmd_buffer->renderCommandEncoder(pass);
        render_encoder->setRenderPipelineState(render_pipeline);
        render_encoder->setVertexBuffer(vertex_pos_buffer_render, 0, 0);
        render_encoder->drawPrimitives(MTL::PrimitiveType::PrimitiveTypeTriangle, NS::UInteger(0), NS::UInteger(3));

        // Start the Dear ImGui frame
        // encoder->pushDebugGroup(NS::String::string("ImGui demo", NS::StringEncoding::UTF8StringEncoding));
        ImGui_ImplMetal_NewFrame(pass);
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::Begin("hello-metal");
        ImGui::End();
        ImGui::Render();
        ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(), cmd_buffer, render_encoder);
        // encoder->popDebugGroup();

        render_encoder->endEncoding();
        cmd_buffer->presentDrawable(surface);
        cmd_buffer->commit();

        surface->release();
        pass->release();
        compute_encoder->release();
        render_encoder->release();
        cmd_buffer->release();
    }

    vertex_pos_buffer->release();
    vertex_pos_buffer_render->release();
    render_pipeline->release();
    queue->release();
    device->release();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
