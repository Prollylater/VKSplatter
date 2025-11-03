
#include "Drawable.h"
#include "RenderQueue.h"

void RenderQueue::build(const Scene &scene)
{
    opaqueObjects.clear();
    opaqueObjects.shrink_to_fit();
    for (const auto &drawable : scene.drawables)
    {
        if (!drawable.visible)
        {
            continue;
        }

        opaqueObjects.push_back(&drawable);
    }
}