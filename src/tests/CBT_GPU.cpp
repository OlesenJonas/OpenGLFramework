#include <glm/integer.hpp>
#include <intern/Misc/OpenGLErrorHandler.h>
#include <intern/Terrain/CBTGPU.h>
#include <intern/Terrain/CBTOptimized.h>
#include <intern/Window/Window.h>

#include <stdlib.h>
#include <testing/misc.h>

#include <GLFW/glfw3.h>

bool compareHeaps(const std::vector<uint32_t>& heap1, const std::vector<uint32_t>& heap2)
{
    for(auto i = 0; i < heap1.size(); i++)
    {
        if(heap1[i] != heap2[i])
        {
            return false;
        }
    }
    return true;
}

int main()
{
    bool success = true;

    // CBTs require a valid OpenGL context
    OpenGLContextForTests GLContext;

    const uint32_t MAX_DEPTH = 18;
    CBTOptimized cbt(MAX_DEPTH);
    CBTGPU cbtgpu(MAX_DEPTH);

    std::vector<uint32_t> cpuHeap = cbt.getHeap();
    std::vector<uint32_t> gpuHeap = cbtgpu.getHeap();

    success &= compareHeaps(cpuHeap, gpuHeap);

    abandonIfFalse(success, GLCONTEXT_CLEANUP(GLContext));

    // randomly refine the trees
    srand(time(nullptr));
    for(int iter = 0; iter < 10; iter++)
    {
        const float xpos = (static_cast<float>(rand()) / RAND_MAX) - 1.0f;
        const float zpos = (static_cast<float>(rand()) / RAND_MAX);

        for(int i = 0; i < 30; i++)
        {
            cbt.refineAroundPoint({xpos, zpos});
            cbtgpu.refineAroundPoint({xpos, zpos});

            // cbt.doSumReduction(); //already part of refine...() for CPU version
            cbtgpu.doSumReduction();
            cbtgpu.writeIndirectCommands();
        }

        cpuHeap = cbt.getHeap();
        gpuHeap = cbtgpu.getHeap();
        success &= compareHeaps(cpuHeap, gpuHeap);
        abandonIfFalse(success, GLCONTEXT_CLEANUP(GLContext));

        const auto cpuamnt = cbt.getAmountOfLeafNodes();
        const auto gpuamnt = cbtgpu.getAmountOfLeafNodes();
        success &= cpuamnt == gpuamnt;
        abandonIfFalse(success, GLCONTEXT_CLEANUP(GLContext));
    }

    GLCONTEXT_CLEANUP(GLContext);
    return 0;
}