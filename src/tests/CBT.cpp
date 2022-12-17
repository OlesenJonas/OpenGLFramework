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

        const uint32_t MAX_DEPTH = 20;
        CBT cbt(MAX_DEPTH);
        CBTOptimized cbtOpt(MAX_DEPTH);
        // do random mutations on both trees
        for(int i = 0; i < 25; i++)
        {
            auto cbtLeafNodes = cbt.getAmountOfLeafNodes();
            auto cbtOptLeafNodes = cbtOpt.getAmountOfLeafNodes();
            success &= expectedValue(cbtLeafNodes, cbtOptLeafNodes);

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

            cbt.doSumReduction();
            cbtOpt.doSumReduction();

            const uint32_t maxNodeIndex = (1u << (MAX_DEPTH + 1u)) - 1u;
            for(uint32_t j = 0; j < maxNodeIndex; j++)
            {
                const uint32_t cbtValue = cbt.heap[j];
                const uint32_t nodeDepth = glm::findMSB(j);
                const uint32_t cbtOptValue = cbtOpt.getNodeValue({j, nodeDepth});

                success &= expectedValue(cbtValue, cbtOptValue);

                abandonIfFalse(success, GLCONTEXT_CLEANUP(GLContext));
            }
        }
    }

    GLCONTEXT_CLEANUP(GLContext);
    return 0;
}