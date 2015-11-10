var io = require('socket.io-client');

var socket = io.connect('http://localhost');

socket.on('port-data', function(data) {
    console.log("\nIncoming Message");
    console.log("- Timestamp:", data.timestamp);
    console.log("- Channel:", data.channel);
    console.log("- Message:", data.message);
    console.log("- Raw:", data.raw);
});

window.dashboard = {
    socket: socket
};
