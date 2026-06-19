(function () {
  const button = document.getElementById("connect");
  const log = document.getElementById("log");

  function append(message) {
    const time = new Date().toISOString();
    log.textContent += `[${time}] ${message}\n`;
  }

  button.addEventListener("click", function () {
    const url = `ws://${window.location.host}/`;
    append(`connecting to ${url}`);

    const socket = new WebSocket(url);

    socket.addEventListener("open", function () {
      append("open: handshake accepted");
      append("waiting for fragmented server message");
    });

    socket.addEventListener("message", function (event) {
      append(`message: ${event.data}`);
    });

    socket.addEventListener("close", function (event) {
      append(`close: code=${event.code} reason="${event.reason}"`);
    });

    socket.addEventListener("error", function () {
      append("error: browser reported a WebSocket failure");
    });
  });
}());
