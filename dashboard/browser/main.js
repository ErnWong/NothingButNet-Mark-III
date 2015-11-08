var io = require('socket.io-client');

var socket = io.connect('http://localhost');
socket.on('test', function (data) {
    console.log(data);
    socket.emit('other event', 'hi');
});
