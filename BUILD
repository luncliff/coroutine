#
#   Bazel build support is not available for now. 
#   this file is for reservation and to prevent future filename collision
#
cc_library(
    name = "coroutine_portable",
    includes = ["include"],
    defines=[
        "_RESUMABLE_FUNCTIONS_SUPPORTED" 
    ],
)

cc_library(
    name = "coroutine_event",
    includes = [
        "include",
        "modules/event"
    ],
    srcs = ["modules/concrt/libmain.cpp"],
    deps = [
        ":coroutine_portable",
    ],
    defines=[
        "FORCE_STATIC_LINK" # for bazel build, use static linkage by default
    ],
    # linkshared=True,
)
