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

MTL::Library* create_shader(MTL::Device* device)
{
    const char* shaderSrc = R"(
        #include <metal_stdlib>
        using namespace metal;

        struct v2f
        {
            float4 position [[position]];
        };

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

MTL::RenderPipelineState* create_pipeline(MTL::Device* device, MTL::Library* library)
{
    MTL::Function* vertex_fn = library->newFunction(NS::String::string("vertexMain", NS::StringEncoding::UTF8StringEncoding));
    MTL::Function* frag_fn = library->newFunction(NS::String::string("fragmentMain", NS::StringEncoding::UTF8StringEncoding));

    MTL::RenderPipelineDescriptor* desc = MTL::RenderPipelineDescriptor::alloc()->init();
    desc->setVertexFunction(vertex_fn);
    desc->setFragmentFunction(frag_fn);
    desc->colorAttachments()->object(0)->setPixelFormat(MTL::PixelFormat::PixelFormatBGRA8Unorm_sRGB);

    NS::Error* error = nullptr;
    MTL::RenderPipelineState* pipeline = device->newRenderPipelineState(desc, &error);
    vertex_fn->release();
    frag_fn->release();
    desc->release();
    if (!pipeline)
    {
        printf("%s", error->localizedDescription()->utf8String());
        return nullptr;
    }
    return pipeline;
}

MTL::Buffer* create_buffer(MTL::Device* device)
{
    float positions[][2] =
    {
        { -0.8f,  -0.8f },
        {  0.0f,   0.8f },
        {  0.8f, - 0.8f }
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
    MTL::ClearColor color{ 1, 0, 0, 1 };

    // Setup style
    ImGui::StyleColorsDark();

    MTL::Library* library = create_shader(device);
    MTL::RenderPipelineState* pipeline = create_pipeline(device, library);
    library->release();

    MTL::Buffer* vertex_pos_buffer = create_buffer(device);

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

        // draw triangle
        MTL::CommandBuffer* buffer = queue->commandBuffer();
        MTL::RenderCommandEncoder* encoder = buffer->renderCommandEncoder(pass);
        encoder->setRenderPipelineState(pipeline);
        encoder->setVertexBuffer(vertex_pos_buffer, 0, 0);
        encoder->drawPrimitives(MTL::PrimitiveType::PrimitiveTypeTriangle, NS::UInteger(0), NS::UInteger(3));

        // Start the Dear ImGui frame
        // encoder->pushDebugGroup(NS::String::string("ImGui demo", NS::StringEncoding::UTF8StringEncoding));
        ImGui_ImplMetal_NewFrame(pass);
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::Begin("hello-metal");
        ImGui::End();
        ImGui::Render();
        ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(), buffer, encoder);
        // encoder->popDebugGroup();

        encoder->endEncoding();
        buffer->presentDrawable(surface);
        buffer->commit();

        surface->release();
        pass->release();
        encoder->release();
        buffer->release();
    }

    vertex_pos_buffer->release();
    pipeline->release();
    queue->release();
    device->release();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
