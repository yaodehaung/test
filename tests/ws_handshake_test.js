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

let response = Buffer.alloc(0);

socket.setTimeout(3000);

socket.on("data", (chunk) => {
  response = Buffer.concat([response, chunk]);
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
  const headerEnd = response.indexOf("\r\n\r\n");
  const header = headerEnd === -1
    ? response.toString("utf8")
    : response.subarray(0, headerEnd + 4).toString("utf8");
  const frames = headerEnd === -1
    ? Buffer.alloc(0)
    : response.subarray(headerEnd + 4);

  if (process.exitCode) {
    return;
  }

  if (!header.startsWith("HTTP/1.1 101 Switching Protocols")) {
    console.error("Expected HTTP 101 Switching Protocols");
    console.error(header);
    process.exitCode = 1;
    return;
  }

  if (!header.includes(`Sec-WebSocket-Accept: ${expectedAccept}`)) {
    console.error(`Expected Sec-WebSocket-Accept: ${expectedAccept}`);
    console.error(header);
    process.exitCode = 1;
    return;
  }

  if (frames.length < 34) {
    console.error("Expected fragmented WebSocket frames after the handshake");
    console.error(frames);
    process.exitCode = 1;
    return;
  }

  const expectedFrames = [
    { first: 0x01, payload: "hello " },
    { first: 0x00, payload: "from " },
    { first: 0x80, payload: "fragmented websocket" },
    { first: 0x88, payload: "" },
  ];
  let offset = 0;

  for (const expected of expectedFrames) {
    const first = frames[offset];
    const len = frames[offset + 1] & 0x7f;
    const payload = frames.subarray(offset + 2, offset + 2 + len)
      .toString("utf8");

    if (first !== expected.first || payload !== expected.payload) {
      console.error("Unexpected WebSocket frame");
      console.error({ first, payload, expected });
      process.exitCode = 1;
      return;
    }

    offset += 2 + len;
  }

  const message = expectedFrames.slice(0, 3)
    .map((frame) => frame.payload)
    .join("");

  if (message !== "hello from fragmented websocket") {
    console.error(`Unexpected fragmented message: ${message}`);
    process.exitCode = 1;
    return;
  }

  console.log("WebSocket handshake and fragmented send test passed");
});
