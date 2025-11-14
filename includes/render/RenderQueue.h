#include "Drawable.h"
#include "Scene.h"


struct RenderQueue
{
    void add(const Drawable *drawable)
    {
        opaqueObjects.push_back(drawable);
    }
    const std::vector<const Drawable *> &getDrawables() const {
        return opaqueObjects;
    };

    std::vector<const Drawable *> opaqueObjects;
    void sortByMaterial();

    // Single renderqueue build
    void build(const RenderScene &scene);

};
