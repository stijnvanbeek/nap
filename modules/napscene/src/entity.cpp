#include <rtti/pythonmodule.h>
#include "entity.h"
#include <nap/core.h>
#include "scene.h"

using namespace std;

RTTI_BEGIN_CLASS(nap::Entity)
	RTTI_PROPERTY("Components", &nap::Entity::mComponents, nap::rtti::EPropertyMetaData::Embedded)
	RTTI_PROPERTY("Children", &nap::Entity::mChildren, nap::rtti::EPropertyMetaData::Default)
RTTI_END_CLASS

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::EntityInstance)
	RTTI_CONSTRUCTOR(nap::Core&, const nap::Entity*)
	RTTI_FUNCTION("findComponent", (nap::ComponentInstance* (nap::EntityInstance::*)(const std::string &) const) &nap::EntityInstance::findComponent)
RTTI_END_CLASS

namespace nap
{
	EntityInstance::EntityInstance(Core& core, const Entity* entity) :
		mCore(&core), mResource(entity)
	{
	}


	bool EntityInstance::init(Scene& scene, EntityCreationParameters& entityCreationParams, utility::ErrorState& errorState)
	{
		for (int index = 0; index < mResource->mChildren.size(); ++index)
		{
			EntityInstance* child_entity_instance = scene.createChildEntityInstance(*mResource->mChildren[index], index, entityCreationParams, errorState);
			if (!errorState.check(child_entity_instance != nullptr, "Failed to spawn child entity %s for entity %s", mResource->mChildren[index]->mID.c_str(), mID.c_str()))
				return false; 

			addChild(*child_entity_instance);
		}

		return true;
	}


	void EntityInstance::update(double deltaTime)
	{
		for (auto& component : mComponents)
			component->update(deltaTime);

		for (EntityInstance* child : mChildren)
			child->update(deltaTime);
	}


	void EntityInstance::addComponent(std::unique_ptr<ComponentInstance> component)
	{
		mComponents.emplace_back(std::move(component));
	}


	ComponentInstance* EntityInstance::findComponent(const std::string& type) const
	{
		return findComponent(rtti::TypeInfo::get_by_name(type));
	}


	ComponentInstance* EntityInstance::findComponent(const rtti::TypeInfo& type, rtti::ETypeCheck typeCheck) const
	{
		ComponentList::const_iterator pos = std::find_if(mComponents.begin(), mComponents.end(), [&](auto& element) 
		{ 
			return rtti::isTypeMatch(element->get_type(), type, typeCheck);
		});
		if (pos == mComponents.end())
			return nullptr;

		return pos->get();
	}


	void EntityInstance::getComponentsOfType(const rtti::TypeInfo& type, std::vector<ComponentInstance*>& components, rtti::ETypeCheck typeCheck) const
	{
		for (auto& component : mComponents)
			if (rtti::isTypeMatch(component->get_type(), type, typeCheck))
				components.emplace_back(component.get());
	}


	bool EntityInstance::hasComponentsOfType(const rtti::TypeInfo& type, rtti::ETypeCheck typeCheck) const
	{
		for (auto& component : mComponents)
			if (rtti::isTypeMatch(component->get_type(), type, typeCheck))
				return true;

		return false;
	}


	bool EntityInstance::hasComponent(const rtti::TypeInfo& type, rtti::ETypeCheck typeCheck) const
	{
		return findComponent(type, typeCheck) != nullptr;
	}


	ComponentInstance& EntityInstance::getComponent(const rtti::TypeInfo& type, rtti::ETypeCheck typeCheck) const
	{
		ComponentInstance* result = findComponent(type, typeCheck);
		assert(result != nullptr);
		return *result;
	}


	void EntityInstance::addChild(EntityInstance& child)
	{
		assert(child.mParent == nullptr);
		child.mParent = this;
		mChildren.emplace_back(&child);
	}


	void EntityInstance::clearChildren()
	{
		for (EntityInstance* child : mChildren)
			child->mParent = nullptr;

		mChildren.clear();
	}


	const EntityInstance::ChildList& EntityInstance::getChildren() const
	{
		return mChildren;
	}


	EntityInstance* EntityInstance::getParent() const
	{
		return mParent;
	}


	const nap::Entity* EntityInstance::getEntity() const
	{
		return mResource;
	}


	Core* EntityInstance::getCore() const
	{
		return mCore;
	}

	//////////////////////////////////////////////////////////////////////////

	ObjectPtr<Component> Entity::findComponent(const rtti::TypeInfo& type, rtti::ETypeCheck typeCheck) const
	{
		ComponentList::const_iterator pos = std::find_if(mComponents.begin(), mComponents.end(), [&](auto& element) { return isTypeMatch(element->get_type(), type, typeCheck); });
		if (pos == mComponents.end())
			return nullptr;

		return pos->get();
	}


	bool Entity::hasComponent(const rtti::TypeInfo& type, rtti::ETypeCheck typeCheck) const
	{
		return findComponent(type, typeCheck) != nullptr;
	}

}