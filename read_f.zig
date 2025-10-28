const std = @import("std");
const fs = std.fs;
const process = std.process;

pub fn main() !void {
    const args = try process.argsAlloc(std.heap.page_allocator);
    defer process.argsFree(std.heap.page_allocator, args);

    var file = if (args.len <= 1) fs.File.stdout() else fs.cwd().openFile(args[1], .{.mode = .read_only}) catch {
        std.log.err("{s}: failed to open file '{s}'\n", .{args[0], args[1]});
        std.process.exit(1);
    };
    defer if (args.len > 1) file.close();

    var read_buffer: [512]u8 = undefined;
    var reader = file.reader(&read_buffer);
    const r = &reader.interface;

    var cur_byte: u8 = 0;
    var n_bits_read: u8 = 0;

    var stdout_buffer: [512]u8 = undefined;
    var stdout_writer = fs.File.stdout().writer(&stdout_buffer);
    const stdout = &stdout_writer.interface;

    while (true) {
        const c = r.takeByte() catch break;
        if (c == ';') {
            _ = r.discardDelimiterInclusive('\n') catch break;
            continue;
        }
        if (c != '0' and c != '1') continue;
        cur_byte = cur_byte*2 + c-'0';
        n_bits_read += 1;
        if (n_bits_read == 8) {
            try stdout.writeByte(cur_byte);
            cur_byte = 0;
            n_bits_read = 0;
        }
    }

    try stdout.flush();
}
