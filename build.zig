const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const exe = b.addExecutable(.{
        .name = "chttp",
        .target = target,
        .optimize = optimize,
    });
    exe.addCSourceFiles(.{
        .root = .{ .path = "src" },
        .files = &[_][]const u8{
            "main.c",
            "http_state_machine.c",
        },
        .flags = &[_][]const u8{
            "-Wall",
            "-Werror",
            "-pedantic",
            "-std=c11",
        },
    });
    exe.addIncludePath(.{ .path = "src" });
    exe.linkLibC();
    exe.linkSystemLibrary("event_core");
    exe.linkSystemLibrary("event_extra");
    exe.linkSystemLibrary("event_extra");
    exe.linkSystemLibrary("event_pthreads");

    b.installArtifact(exe);

    const run_cmd = b.addRunArtifact(exe);

    run_cmd.step.dependOn(b.getInstallStep());

    if (b.args) |args| {
        run_cmd.addArgs(args);
    }

    const run_step = b.step("run", "Run the app");
    run_step.dependOn(&run_cmd.step);
}
