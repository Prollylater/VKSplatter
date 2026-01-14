
#include "Drawable.h"
#include "RenderQueue.h"

void RenderQueue::build(const RenderScene &scene)
{
    opaqueObjects.clear();
    opaqueObjects.shrink_to_fit();
    for (const auto &drawable : scene.drawables)
    {
        opaqueObjects.push_back(&drawable);
    }
}