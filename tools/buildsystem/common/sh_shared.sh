configure_python() {
    nap_root=$1
    thirdparty="$nap_root/thirdparty"
    if [ "$(uname)" = "Linux" ]; then
        case "$(arch)" in
        "x86_64")
            nap_arch="x86_64"
            ;;
        "aarch64")
            if [ "$(getconf LONG_BIT)" = "64" ]; then
                nap_arch="arm64"
            else
                nap_arch="armhf"
            fi
            ;;
        *)
            nap_arch="armhf"
            ;;
        esac
        python="python3"
    else
        python="python3"
    fi
}
