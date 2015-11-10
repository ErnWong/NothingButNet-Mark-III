var express = require('express');
var app = express();
var server = require('http').Server(app);
var io = require('socket.io')(server);

server.listen(80);

app.use(express.static('public'));

io.on('connection', function (socket) {
    socket.emit('test', { hello: 'world' });
    socket.on('other event', function (data) {
        console.log(data);
    });
});
