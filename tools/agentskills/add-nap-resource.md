---
name: add-nap-resource
description: |
  Adds an object to a NAP json file. This is a file that is deserialized by the nap::ResourceManager. As its root it has an "Objects" array that contains all the resources that will be deserialized.
  Triggers when: 
  - the user asks "Add a <type> resource <name> to <app> in <container>"
  - the user asks "Add a <type> component <name> to <app> in <container>"
  Both <name> and <container> are optional.
---

## Steps

1. Locate the json <file> to edit. If not explicitly specified look for it in the data subdirectory of the specified nap <app>, where it is commonly named app_structur.json, objects.json or <app>.json.
2. If the container is specified, look for an object whose "mID" property matches the container.
Look for the C++ header file containing the class specified in the "Type" field of the container.
Look for an embedded array property in the container class's RTTI definitions, typically it's named "Children", "Objects", or "Components".
3. In the json file create a new object element in the array field corresponding to the embedded array property of the container. Set the "Type" field of this object to <type> and the "mID" field to <name>. Make sure to prepend C++ namespaces to the <type> by looking the C++ header file for this type up.
4. Test the change in <file>. Don't fix errors about required properties not being found. If there is a duplicate name or mID, add a number to the "mID" proeprty until the error is gone.
