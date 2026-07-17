const std = @import("std");

const c = @cImport({
    @cInclude("fcntl.h");
    @cInclude("termios.h");
    @cInclude("unistd.h");
    @cInclude("string.h");
});

pub fn main(init: std.process.Init) !void {
    const io = init.io;
    const fd = c.open("/dev/ttyACM0", c.O_RDWR | c.O_NOCTTY);
    if (fd < 0) {
        std.debug.print("open failed\n", .{});
        return;
    }
    defer _ = c.close(fd);

    var tty: c.struct_termios = undefined;

    if (c.tcgetattr(fd, &tty) != 0) {
        std.debug.print("tcgetattr failed\n", .{});
        return;
    }

    _ = c.cfsetispeed(&tty, c.B115200);
    _ = c.cfsetospeed(&tty, c.B115200);

    tty.c_cflag |= c.CLOCAL | c.CREAD;
    tty.c_cflag &= ~@as(c_uint, c.CSIZE);
    tty.c_cflag |= c.CS8;
    tty.c_cflag &= ~@as(c_uint, c.PARENB);
    tty.c_cflag &= ~@as(c_uint, c.CSTOPB);
    tty.c_cflag &= ~@as(c_uint, c.CRTSCTS);

    tty.c_lflag = 0;
    tty.c_iflag = 0;
    tty.c_oflag = 0;

    if (c.tcsetattr(fd, c.TCSANOW, &tty) != 0) {
        std.debug.print("tcsetattr failed\n", .{});
        return;
    }

    var stdin = std.Io.File.stdin();
    var buf: [256]u8 = undefined;
    var reader = stdin.reader(io, &buf);
    while (true) {
        const line = try reader.interface.takeDelimiterInclusive('\n');
        _ = c.write(fd, line.ptr, line.len);
    }
}
