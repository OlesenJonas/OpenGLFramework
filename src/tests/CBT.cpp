#include <glm/integer.hpp>
#include <intern/Misc/OpenGLErrorHandler.h>
#include <intern/Terrain/CBT.h>
#include <intern/Terrain/CBTOptimized.h>
#include <intern/Window/Window.h>

#include <testing/misc.h>

#include <GLFW/glfw3.h>

int main()
{
    // CBTs require a valid OpenGL context
    OpenGLContextForTests GLContext;

    for(int i = 6; i < 25; i++)
    {
        CBT cbt(i);
    }

    {
        bool success = true;

        const uint32_t MAX_DEPTH = 18;
        CBT cbt(MAX_DEPTH);
        CBTOptimized cbtOpt(MAX_DEPTH);

        std::vector<uint32_t> heapBackup;

        srand(time(nullptr));

        // do random mutations on both trees
        for(int iter = 0; iter < 50; iter++)
        {
            auto cbtLeafNodes = cbt.getAmountOfLeafNodes();
            auto cbtOptLeafNodes = cbtOpt.getAmountOfLeafNodes();
            success &= expectedValue(cbtOptLeafNodes, cbtLeafNodes);

            abandonIfFalse(success, GLCONTEXT_CLEANUP(GLContext));

            const uint32_t leafIndex = uint32_t(rand()) % cbtLeafNodes;
            const CBT::Node node = cbt.leafIndexToNode(leafIndex);
            const CBTOptimized::Node nodeOpt = cbtOpt.leafIndexToNode(leafIndex);
            const bool doSplit = (rand() % 2) == 0;
            if(doSplit)
            {
                cbt.splitNodeConforming(node);
                cbtOpt.splitNodeConforming(nodeOpt);
            }
            else
            {
                cbt.mergeNodeConforming(node);
                cbtOpt.mergeNodeConforming(nodeOpt);
            }

            // check if leaf nodes are still the same
            {
                const uint32_t level = MAX_DEPTH;

                const uint32_t loopStart = 1u << level;
                const uint32_t loopEnd = 1u << (level + 1u);
                for(uint32_t j = loopStart; j < loopEnd; j++)
                {
                    const uint32_t localIndex = j - loopStart;

                    const uint32_t cbtValue = cbt.heap[j];
                    const uint32_t nodeDepth = glm::findMSB(j);
                    assert(nodeDepth == level);
                    const uint32_t cbtOptValue = cbtOpt.getNodeValue({j, nodeDepth});

                    success &= expectedValue(cbtOptValue, cbtValue);

                    abandonIfFalse(success, GLCONTEXT_CLEANUP(GLContext));
                }
            }

            cbt.doSumReduction();
            heapBackup = cbtOpt.getHeap();
            // cbtOpt.doSumReduction();
            cbtOpt.doSumReductionOptimized();

            // check if interior nodes are the same after sum reduction
            for(uint32_t level = MAX_DEPTH - 1; level <= MAX_DEPTH; level--)
            {
                const uint32_t loopStart = 1u << level;
                const uint32_t loopEnd = 1u << (level + 1u);
                for(uint32_t j = loopStart; j < loopEnd; j++)
                {
                    const uint32_t localIndex = j - loopStart;

                    const uint32_t cbtValue = cbt.heap[j];
                    const uint32_t nodeDepth = glm::findMSB(j);
                    assert(nodeDepth == level);
                    const uint32_t cbtOptValue = cbtOpt.getNodeValue({j, nodeDepth});

                    success &= expectedValue(cbtOptValue, cbtValue);

                    if(!success)
                    {
                        const uint32_t arrIndex = cbtOpt.getNodeBitIndex({j, nodeDepth}) / 32u;
                        const uint32_t bitIndex = cbtOpt.getNodeBitIndex({j, nodeDepth}) % 32u;
                        const uint32_t beforeHeapEntry = heapBackup[arrIndex];
                        cbtOpt.setHeap(heapBackup);
                        cbtOpt.doSumReductionOptimized();
                    }

                    abandonIfFalse(success, GLCONTEXT_CLEANUP(GLContext));
                }
            }
        }
    }

    GLCONTEXT_CLEANUP(GLContext);
    return 0;
}