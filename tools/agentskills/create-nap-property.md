---
name: create-nap-property
description: |
  Create a property in a nap Resource class.
  Triggers when the user types:
  - Create a property <prop_name> in <class>
  - Add a property <prop_name> in <class>
---

1. Create public member variable m<prop_name> of type <type> in <class> 

3. add between RTTI_BEGIN_CLASS and RTTI_END_CLASS
'''
RTTI_PROPERTY("<prop_name>", nap::<class>::m<prop_name>, nap::rtti::EPropertyMetaData::Default)
'''
If the property is not optional use Required instead of Default.
Don't implement any behaviour yes unless specified.
