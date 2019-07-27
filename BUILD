cc_library(
    name = "coroutine_portable",
    includes = ["include"],
)

cc_library(
    name = "coroutine_concrt",
    includes = [
        "include",
        "modules/concrt"
    ],
    srcs = ["modules/concrt/libmain.cpp"],
    deps = [
        ":coroutine_portable",
    ],
    defines=[
        "FORCE_STATIC_LINK" # for bazel build, use static linkage by default
    ],
)

# cc_binary(
#     name = "coroutine_test_suite",
#     includes = ["test"],
#     srcs = ["test/test_main.cpp"],
#     deps = [
#         ":coroutine_portable",
#     ],
#     # linkshared=True,
# )
