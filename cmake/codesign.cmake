if (APPLE)
    set(signature "-") # Use add hoc sign as default
    if (DEFINED ENV{MACOS_CODE_SIGNATURE})
        set(signature $ENV{MACOS_CODE_SIGNATURE}) # If defined, use environment variable as signature
    endif ()
    # Codesign the file
    execute_process(COMMAND codesign --force -s ${signature} ${path})
endif ()
