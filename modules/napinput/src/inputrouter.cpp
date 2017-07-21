// local includes
#include <inputrouter.h>
#include "inputevent.h"
#include "inputcomponent.h"
#include "nap/entity.h"

RTTI_BEGIN_CLASS(nap::DefaultInputRouterComponent)
RTTI_END_CLASS

RTTI_BEGIN_CLASS_CONSTRUCTOR1(nap::DefaultInputRouterComponentInstance, nap::EntityInstance&)
RTTI_END_CLASS

namespace nap
{

	void DefaultInputRouter::routeEvent(const InputEvent& event, const EntityList& entities)
	{
		for (EntityInstance* entity : entities)
		{
			std::vector<InputComponentInstance*> input_components;
			entity->getComponentsOfType<InputComponentInstance>(input_components);

			for (InputComponentInstance* component : input_components)
			{
				component->trigger(event);
			}
		}
	}

}

