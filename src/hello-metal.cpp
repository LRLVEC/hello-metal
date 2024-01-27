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


static void quit(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

int main(void)
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(640, 480, "GLFW Metal", NULL, NULL);

    MTL::Device* gpu = MTL::CreateSystemDefaultDevice();
    MTL::CommandQueue* queue = gpu->newCommandQueue();
    CA::MetalLayer* swapchain = (CA::MetalLayer*)glfwGetCocoaMetalLayer(window);

    swapchain->setDevice(gpu);
    // swapchain->opaque = true;

    glfwSetKeyCallback(window, quit);
    MTL::ClearColor color{ 1, 0, 0, 1 };

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        color.red = (color.red > 1.0) ? 0 : color.red + 0.01;
        CA::MetalDrawable* surface = swapchain->nextDrawable();
        MTL::RenderPassDescriptor* pass = MTL::RenderPassDescriptor::alloc();
        pass->init();
        pass->colorAttachments()->object(0)->setClearColor(color);
        pass->colorAttachments()->object(0)->setLoadAction(MTL::LoadActionClear);
        pass->colorAttachments()->object(0)->setStoreAction(MTL::StoreActionStore);
        pass->colorAttachments()->object(0)->setTexture(surface->texture());

        MTL::CommandBuffer* buffer = queue->commandBuffer();
        MTL::RenderCommandEncoder* encoder = buffer->renderCommandEncoder(pass);
        encoder->endEncoding();
        buffer->presentDrawable(surface);
        buffer->commit();
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
