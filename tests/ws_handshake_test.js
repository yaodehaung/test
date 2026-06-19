const crypto = require("crypto");
const net = require("net");

const host = process.env.WS_TEST_HOST || "127.0.0.1";
const port = Number(process.env.WS_TEST_PORT || "8080");
const key = "dGhlIHNhbXBsZSBub25jZQ==";
const guid = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
const expectedAccept = crypto
  .createHash("sha1")
  .update(key + guid)
  .digest("base64");

const request = [
  "GET / HTTP/1.1",
  `Host: ${host}:${port}`,
  "Upgrade: websocket",
  "Connection: Upgrade",
  `Sec-WebSocket-Key: ${key}`,
  "Sec-WebSocket-Version: 13",
  "",
  "",
].join("\r\n");

const socket = net.createConnection({ host, port }, () => {
  socket.write(request);
});

let response = "";

socket.setTimeout(3000);

socket.on("data", (chunk) => {
  response += chunk.toString("utf8");
  if (response.includes("\r\n\r\n")) {
    socket.end();
  }
});

socket.on("timeout", () => {
  console.error("Timed out waiting for WebSocket handshake response");
  socket.destroy();
  process.exitCode = 1;
});

socket.on("error", (error) => {
  console.error(`Connection failed: ${error.message}`);
  process.exitCode = 1;
});

socket.on("close", () => {
  if (process.exitCode) {
    return;
  }

  if (!response.startsWith("HTTP/1.1 101 Switching Protocols")) {
    console.error("Expected HTTP 101 Switching Protocols");
    console.error(response);
    process.exitCode = 1;
    return;
  }

  if (!response.includes(`Sec-WebSocket-Accept: ${expectedAccept}`)) {
    console.error(`Expected Sec-WebSocket-Accept: ${expectedAccept}`);
    console.error(response);
    process.exitCode = 1;
    return;
  }

  console.log("WebSocket handshake test passed");
});
