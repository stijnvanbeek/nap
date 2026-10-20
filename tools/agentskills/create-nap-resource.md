---
name: create-nap-resource
description: |
  Create a new resource class for a NAP framework project
  Triggers when the user types:
  - Create a resource <name> in <module_dir>
  - Create a resource <name> in <module_dir> derived from <base_class>
---

## steps

1. Create <name>.h and <name>.cpp in <module_dir>/src.
2. In <name>.h add #pragma once, include <nap/resource.h> and define a class <name> in namespace nap. Don't include anything else unless specified.
3. Derive the class from <base_class>. Use the RTTI_ENABLE(<base_class>) macro on the first line after the {. If base classed not specified use Resource. 
5. Override the init method from Resource the class.
4. Add the NAPAPI macro between the class keyword and the class name.
5. In the cpp file
'''
#include <<name>.h>
'''

'''
RTTI_BEGIN_CLASS(nap::<name>)
RTTI_END_CLASS
'''

6. Create properties in <name> when explicitly asked. 

Don't implement anything unless explicitly asked.
