#include <algorithm>
#include <nap/entity.h>
#include <nap/component.h>
#include <nap/serializer.h>

using namespace std;

// Define Entity in Type Registry
RTTI_DEFINE(nap::Entity)

namespace nap
{
	Entity* Entity::getRoot()
	{
		Entity* parent = getParent();
		if (!parent) return this;
		while (parent->hasParent())
			parent = parent->getParent();
		return parent;
	}
    
    
    // Add a new component of @componentType
    Component& Entity::addComponent(const RTTI::TypeInfo& componentType)
    {
        assert(componentType.isKindOf<Component>());
        return *static_cast<Component*>(&addChild("", componentType));
    }
    
    // Add a component from somewhere else, forwarding parentship to this entity
    Component& Entity::addComponent(std::unique_ptr<Component> component)
    {
        return *static_cast<Component*>(&addChild(std::move(component)));
    }
    
    
    // Remove a component from this entity
    bool Entity::removeComponent(Component& comp)
    {
        return removeChild(comp);
    }
    
    
    Component* Entity::getComponentOfType(const RTTI::TypeInfo& componentType)
    {
        return dynamic_cast<Component*>(getChildOfType(componentType));
    }
    
 
    Entity& Entity::addEntity(const std::string& name)
    {
        std::unique_ptr<Entity> e = std::unique_ptr<Entity>(new Entity(mCore));
        e->mName = name;
        return static_cast<Entity&>(addChild(std::move(e)));
    }
    
    
}