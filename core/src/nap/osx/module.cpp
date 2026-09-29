/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

// Local Includes
#include <nap/module.h>
#include <nap/logger.h>

// External Includes
#include <utility/fileutils.h>
#include <assert.h>

#include <dlfcn.h> // Posix shared object loading
#import <mach-o/dyld.h>
#import <mach-o/nlist.h>
#import <stdio.h>
#import <string.h>

#ifdef __LP64__
typedef struct mach_header_64 mach_header_t;
typedef struct segment_command_64 segment_command_t;
typedef struct nlist_64 nlist_t;
#else
typedef struct mach_header mach_header_t;
typedef struct segment_command segment_command_t;
typedef struct nlist nlist_t;
#endif

namespace dllhandle
{

	static const char * first_external_symbol_for_image(const mach_header_t *header)
	{
	    Dl_info info;
	    if (dladdr(header, &info) == 0)
	        return NULL;

	    segment_command_t *seg_linkedit = NULL;
	    segment_command_t *seg_text = NULL;
	    struct symtab_command *symtab = NULL;

	    struct load_command *cmd = (struct load_command *)((intptr_t)header + sizeof(mach_header_t));
	    for (uint32_t i = 0; i < header->ncmds; i++, cmd = (struct load_command *)((intptr_t)cmd + cmd->cmdsize))
	    {
	        switch(cmd->cmd)
	        {
	            case LC_SEGMENT:
	            case LC_SEGMENT_64:
	                if (!strcmp(((segment_command_t *)cmd)->segname, SEG_TEXT))
	                    seg_text = (segment_command_t *)cmd;
	                else if (!strcmp(((segment_command_t *)cmd)->segname, SEG_LINKEDIT))
	                    seg_linkedit = (segment_command_t *)cmd;
	                break;

	            case LC_SYMTAB:
	                symtab = (struct symtab_command *)cmd;
	                break;
	        }
	    }

	    if ((seg_text == NULL) || (seg_linkedit == NULL) || (symtab == NULL))
	        return NULL;

	    intptr_t file_slide = ((intptr_t)seg_linkedit->vmaddr - (intptr_t)seg_text->vmaddr) - seg_linkedit->fileoff;
	    intptr_t strings = (intptr_t)header + (symtab->stroff + file_slide);
	    nlist_t *sym = (nlist_t *)((intptr_t)header + (symtab->symoff + file_slide));

	    for (uint32_t i = 0; i < symtab->nsyms; i++, sym++)
	    {
	        if ((sym->n_type & N_EXT) != N_EXT || !sym->n_value)
	            continue;

	        return (const char *)strings + sym->n_un.n_strx;
	    }

	    return NULL;
	}

	const char * pathname_for_handle(void *handle)
	{
	    for (int32_t i = _dyld_image_count(); i >= 0 ; i--)
	    {
	        const char *first_symbol = first_external_symbol_for_image((const mach_header_t *)_dyld_get_image_header(i));
	        if (first_symbol && strlen(first_symbol) > 1)
	        {
	            handle = (void *)((intptr_t)handle | 1); // in order to trigger findExportedSymbol instead of findExportedSymbolInImageOrDependentImages. See `dlsym` implementation at http://opensource.apple.com/source/dyld/dyld-239.3/src/dyldAPIs.cpp
	            first_symbol++; // in order to remove the leading underscore
	            void *address = dlsym(handle, first_symbol);
	            Dl_info info;
	            if (dladdr(address, &info))
	                return info.dli_fname;
	        }
	    }
	    return NULL;
	}
}

namespace nap
{
	void initModules()
	{ }


	void* loadModule(const nap::ModuleInfo& modInfo, const std::string& library, std::string& outLocation, std::string& errorString)
	{
		// Attempt to load the library using default OS system mapping
		// If that fails attempt to locate it using library search paths
		void* handle = dlopen(library.c_str(), RTLD_LAZY);
		if (handle == nullptr)
		{
			for (const auto& path : modInfo.mLibSearchPaths)
			{
				// Find it
				std::string full_path = utility::forceSeparator(utility::stringFormat("%s/%s",
					path.c_str(), library.c_str()));
				if (!utility::fileExists(full_path))
					continue;

				// Load it
				Logger::debug("Explicitly loading library: %s", full_path.c_str());
				handle = dlopen(full_path.c_str(), RTLD_LAZY);
				if (handle != nullptr)
					break;
			}
		}

		// Ensure we have a handle
		if (handle == nullptr)
		{
			errorString = utility::stringFormat("dlopen for library '%s' failed:\n%s", library.c_str(), dlerror());
			return nullptr;
		}

		// Get library load origin
		// char path[PATH_MAX];
		// if (dlinfo(handle, RTLD_DI_ORIGIN, &path) == -1)
		// {
		// 	errorString = utility::stringFormat("origin resolve for '%s' failed:\n%s", library.c_str(), dlerror());
		// 	return nullptr;
		// }
		std::string path = dllhandle::pathname_for_handle(handle);
		if (path.empty())
		{
			errorString = utility::stringFormat("origin resolve for '%s' failed:\n%s", library.c_str(), dlerror());
			return nullptr;
		}

		// Set out location
		outLocation = path;
		return handle;
	}


	void unloadModule(void* module)
	{
		dlclose(module);
	}


	void* findSymbolInModule(void* module, const char* symbolName)
	{
		return dlsym(module, symbolName);
	}


	std::string getModuleExtension()
	{
		return "dylib";
	}
}
