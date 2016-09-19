#include <nap/serviceablecomponent.h>
#include <nap/entity.h>
#include <nap/service.h>

namespace nap
{
	// Constructor
	ServiceableComponent::ServiceableComponent()
	{
		added.connect(mAdded);
	}


	// Registers itself with the correct associated service
	void ServiceableComponent::registerWithService(const Object& object)
	{
		nap::Entity* parent = this->getParent();
		assert(parent != nullptr);

		RTTI::TypeInfo type_info = getTypeInfo();
		nap::Service* c_service = parent->getCore().getServiceForType(type_info);
		if (c_service == nullptr)
		{
			Logger::warn("Unable to register object of type: " + type_info.getName() + " with service, type not registered");
			return;
		}

		// Register
		c_service->registerObject(*this);

		// Set service
		mService = c_service;

		// Call that we've been registered
		registered();
	}
}

RTTI_DEFINE(nap::ServiceableComponent)