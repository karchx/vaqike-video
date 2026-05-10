load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")

cc_library(
    name = "demuxer_lib",
    srcs = ["demuxer.cc"],
    hdrs = ["include/demuxer.h"],
)

cc_library(
    name = "decoder_lib",
    srcs = ["decoder.cc"],
    hdrs = ["include/decoder.h"],
)

cc_binary(
    name = "vaqike",
    srcs = ["main.cc"],
    deps = [":demuxer_lib", ":decoder_lib"],
    linkopts = [
        "-lwebpdemux",
        "-lwebp",
        "-lavformat",
        "-lavcodec",
        "-lswscale",
        "-lavutil",
        "-lx264",
        "-lpthread",
        "-lm",
    ],
)
