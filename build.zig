const std = @import("std");

const mml_lib_source = [_][]const u8{
    "src/eval.c",
    "src/expr.c",
    "src/parser.c",
    "src/config.c",

    "lib/math.c",
    "lib/stdmml.c",

    "c-hashmap/map.c",

    "src/arena.c",
};
const mml_exe_source = [_][]const u8{
    "src/main.c",
    "src/prompt.c",
};

const include_path: []const u8 = "incl";
const c_hashmap_include_path: []const u8 = "c-hashmap";

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});
    const is_debug = b.option(bool, "debug", "should build with debug output and '-g' compiler option (separate from -d output on the executable?)");

    var c_compiler_flags = [_][]const u8{
        "-Wall",
        "-Wextra",
        "-Wno-date-time",
        "-std=c23",
        "-g",
    };

    //if (is_debug != null and is_debug.?) c_compiler_flags[4] = "-g";

    // MML LIBRARY
    var libmml_mod = b.createModule(.{
        .root_source_file = null, // C project
        .target = target,
        .optimize = optimize,
        .link_libc = true,
    });
    const libmml = b.addLibrary(.{
        .name = "mml",
        .linkage = .static,
        .root_module = libmml_mod,
    });
    // source + include
    libmml.installHeadersDirectory(b.path(include_path), "", .{});
    if (is_debug == null or !is_debug.?) libmml_mod.addCMacro("NDEBUG", "");
    libmml_mod.addIncludePath(b.path(include_path));
    libmml_mod.addIncludePath(b.path(c_hashmap_include_path));
    libmml_mod.addCSourceFiles(.{
        .files = &mml_lib_source,
        .flags = &c_compiler_flags,
    });

    b.installArtifact(libmml);


    // MML EXECUTABLE
    var mml_exe_mod = b.createModule(.{
        .root_source_file = null, // C project
        .target = target,
        .optimize = optimize,
        .link_libc = true,
    });
    const mml_exe = b.addExecutable(.{
        .name = "mml",
        .root_module = mml_exe_mod,
    });
    mml_exe_mod.addIncludePath(b.path(include_path));
    mml_exe_mod.addIncludePath(b.path(c_hashmap_include_path));
    mml_exe_mod.addCSourceFiles(.{
        .files = &mml_exe_source,
        .flags = &c_compiler_flags,
    });
    mml_exe_mod.linkLibrary(libmml);

    b.installArtifact(mml_exe);
}
