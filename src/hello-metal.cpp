#define GLFW_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>

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

void create_shader(MTL::Device* device)
{
    const char* shaderSrc = R"(
        #include <metal_stdlib>
        using namespace metal;

        struct v2f
        {
            float4 position [[position]];
            half3 color;
        };

        v2f vertex vertexMain( uint vertexId [[vertex_id]],
                               device const float3* positions [[buffer(0)]],
                               device const float3* colors [[buffer(1)]] )
        {
            v2f o;
            o.position = float4( positions[ vertexId ], 1.0 );
            o.color = half3 ( colors[ vertexId ] );
            return o;
        }

        half4 fragment fragmentMain( v2f in [[stage_in]] )
        {
            return half4( in.color, 1.0 );
        }
    )";

    NS::Error* pError = nullptr;
    MTL::Library* pLibrary = device->newLibrary(NS::String::string(shaderSrc, NS::StringEncoding::UTF8StringEncoding), nullptr, &pError);
    if (!pLibrary)
    {
        printf("%s", pError->localizedDescription()->utf8String());
        assert(false);
    }

    MTL::Function* pVertexFn = pLibrary->newFunction(NS::String::string("vertexMain", NS::StringEncoding::UTF8StringEncoding));
    MTL::Function* pFragFn = pLibrary->newFunction(NS::String::string("fragmentMain", NS::StringEncoding::UTF8StringEncoding));

    MTL::RenderPipelineDescriptor* pDesc = MTL::RenderPipelineDescriptor::alloc()->init();
    pDesc->setVertexFunction(pVertexFn);
    pDesc->setFragmentFunction(pFragFn);
    pDesc->colorAttachments()->object(0)->setPixelFormat(MTL::PixelFormat::PixelFormatBGRA8Unorm_sRGB);

    MTL::RenderPipelineState* _pPSO = device->newRenderPipelineState(pDesc, &pError);
    if (!_pPSO)
    {
        printf("%s", pError->localizedDescription()->utf8String());
        assert(false);
    }

    pVertexFn->release();
    pFragFn->release();
    pDesc->release();
    pLibrary->release();
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

    MTL::Device* gpu = MTL::CreateSystemDefaultDevice();
    MTL::CommandQueue* queue = gpu->newCommandQueue();
    swapchain = (CA::MetalLayer*)glfwGetCocoaMetalLayer(window);
    swapchain->setDevice(gpu);
    // swapchain->opaque = true;

    ImGui_ImplGlfw_InitForOther(window, true);
    ImGui_ImplMetal_Init(gpu);

    glfwSetKeyCallback(window, quit);
    glfwSetFramebufferSizeCallback(window, resize);
    MTL::ClearColor color{ 1, 0, 0, 1 };

    // Setup style
    ImGui::StyleColorsDark();

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


        MTL::CommandBuffer* buffer = queue->commandBuffer();
        MTL::RenderCommandEncoder* encoder = buffer->renderCommandEncoder(pass);
        NS::String* ahh = NS::String::alloc()->init("ImGui demo", NS::StringEncoding::UTF8StringEncoding);
        encoder->pushDebugGroup(ahh);

        // Start the Dear ImGui frame
        ImGui_ImplMetal_NewFrame(pass);
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::Begin("hello-metal");
        ImGui::End();
        ImGui::Render();

        ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(), buffer, encoder);

        encoder->popDebugGroup();
        encoder->endEncoding();
        buffer->presentDrawable(surface);
        buffer->commit();

        surface->release();
        pass->release();
        encoder->release();
        buffer->release();
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    queue->release();
    gpu->release();

    return 0;
}
