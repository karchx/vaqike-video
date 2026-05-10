load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")

cc_library(
    name = "demuxer_lib",
    srcs = ["demuxer.cc"],
    hdrs = ["include/demuxer.h"],
)

cc_library(
    name = "muxer_lib",
    srcs = ["muxer.cc"],
    hdrs = ["include/muxer.h"],
)

cc_library(
    name = "decoder_lib",
    srcs = ["decoder.cc"],
    hdrs = ["include/decoder.h"],
)

cc_library(
    name = "encoder_lib",
    srcs = ["encoder.cc"],
    hdrs = ["include/encoder.h"],
)

cc_library(
    name = "scaler_lib",
    srcs = ["scaler.cc"],
    hdrs = ["include/scaler.h"],
)

cc_library(
    name = "queue_lib",
    srcs = ["queue.cc"],
    hdrs = ["include/queue.h"],
)

cc_binary(
    name = "vaqike",
    srcs = ["main.cc"],
    deps = [":demuxer_lib", ":muxer_lib", ":decoder_lib", ":encoder_lib", ":scaler_lib", ":queue_lib"],
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
