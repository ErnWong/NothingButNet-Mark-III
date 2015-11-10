var express = require('express');
var app = express();
var server = require('http').Server(app);
var io = require('socket.io')(server);
var serialport = require('serialport');
var SerialPort = serialport.SerialPort;

var _ require('lodash');

server.listen(80);

app.use(express.static('public'));

var connectedPorts = Object.create(null);

io.on('connection', function (socket) {
    var attachedPort = '';
    socket.on('list-ports', function () {
        serialPort.list(function (err, ports) {
            socket.emit('port-list', ports);
        });
    });
    socket.on('open-port', function (port) {
        if (attachedPort) {
            socket.leave(attachedPort);
        }
        if (!connectedPorts[port]) {
            connect(port);
        }
        socket.join(port);
        attachedPort = port;
    });
    socket.on('close-port', function () {
        connectedPorts[port].close();
        attachedPort = '';
    });
    socket.on('send-command', function (command) {
        if (!attachedPort) return;
        if (!connectedPorts[attachedPort]) return;
        var sp = connectedPorts[attachedPort];
        sp.write(command.channel + " " + command.message);
    });
});

function connect(port) {
    var sp = new SerialPort(port, {
        baudrate:  115200,
        parser: serialport.parsers.readline('\n')
    });
    sp.on('open', function () {
        connectedPorts[port] = sp;
        sp.on('data', function (data) {
            var info = decode(data);
            io.to(port).emit('port-data', info);
        });
    });
    sp.on('close', function () {
        delete connectedPorts[port];
        io.to(port).emit('port-close');
    });
}


function decode(input) {
    input = input.trim();
    var timestamp = '';
    var channel = '';
    var message = '';

    var sigStart = input.indexOf('[');
    var sigMid = -1;
    var sigEnd = -1;

    if (sigStart < 0) return createInfo(timestamp, channel, message, raw);

    sigMid = input.indexOf('|', sigStart);
    sigEnd = input.indexOf(']', sigStart);

    if (sigEnd < 0) return createInfo(timestamp, channel, message, raw);
    if (sigMid > sigEnd) sigMid = sigStart;

    timestamp = input.substring(sigStart + 1, sigMid).trim();
    channel = input.substring(sigMid + 1, sigEnd).trim();
    message = input.substring(sigEnd + 1).trim();

    return createInfo(timestamp, channel, message, raw);
}


function createInfo(timestamp, channel, message, raw) {
    return {
        timestamp: +timestamp,
        channel: channel,
        message: message,
        raw: raw
    };
}
