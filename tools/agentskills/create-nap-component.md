---
name: create-nap-component
description: |
  Create a new component class for a NAP framework project.
    Components are composites of an entity. Entities and their components are defined in a resource that can be instanced. Therefore both entities and resources have a resource class and an instance class. The entity is spawned using a resource as input to create an entity instance.
  
  Triggers when the user types:
    - Create a component <name> in <module_dir>
    - Create a component <name> in <module_dir> derived from <base_class>
---

## Steps

1. Create a resource <name> in <module_dir> derived from <base_class>. If <base_class> is not specified derive from Component. Define a default constructor.

2. In the header before the class definitions forward declare the component instance:

````
class <name>Instance;
````

3. Below the RTTI_ENABLE() macro put the following macro:
   ````
   DECLARE_COMPONENT(<name>, <name>Instance)
   ````

4. In the header below the component class define the component instance class called <name>Instance.

5. Put the RTTI_ENABLE(<base_class>Instance) on the first line of the instance class definition below the {

6. Define a constructor for the instance class:
   ````
   <name>Instance(EntityInstance& entity, Component& resource) : <base_class>Instance(entity, resource) { }
   ````

7. Define an init override:
````
   bool init(utility::ErrorState& errorState) { return bool; }
````

8. Define an update override:
````
   void update(double deltaTime) override { }
````

9. in the cpp below the RTTI definitions for the component class <name>, put the RTTI definitions for the component instance class:

````
RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::<name>Instance)
RTTI_CONSTRUCTOR(nap::EntityInstance&, nap::Component&)
RTTI_END_CLASS
````
