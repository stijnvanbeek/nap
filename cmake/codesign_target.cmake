if (APPLE)
    set(signature "-") # Use add hoc sign as default
    if (DEFINED ENV{MACOS_CODE_SIGNATURE})
        set(signature $ENV{MACOS_CODE_SIGNATURE}) # If defined, use environment variable as signature
    endif ()
    # Codesign the target post build
    add_custom_command(TARGET ${target} POST_BUILD
            COMMAND codesign --force -s ${signature} $<TARGET_FILE:${target}>)
endif ()
