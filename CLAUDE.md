# 100% Pure C Dependency-Free Express Engine

這是一個使用 **純 C 語言** 撰寫的簡易 Web API Server 範例，整合了：

- 自製 Hash Map 快取系統，概念類似 Mini Redis
- 自製簡易 JSON 欄位解析器
- 自製 Mini Express 風格路由框架
- Linux `epoll` 非阻塞事件驅動伺服器
- REST API 形式的快取存取介面

程式目標是展示如何在不依賴第三方函式庫的情況下，用 C 實作一個簡單的 Web API 引擎。

---

## 1. 系統架構

整體程式可以分成四個主要區塊：

```text
┌──────────────────────────────┐
│ HTTP Client / curl / Browser │
└───────────────┬──────────────┘
                │
                ▼
┌──────────────────────────────┐
│ epoll TCP Server             │
│ - socket                     │
│ - bind/listen                │
│ - accept                     │
│ - non-blocking read          │
└───────────────┬──────────────┘
                │
                ▼
┌──────────────────────────────┐
│ Mini Express Router          │
│ - method match               │
│ - path match                 │
│ - dispatch handler           │
└───────────────┬──────────────┘
                │
                ▼
┌──────────────────────────────┐
│ API Handler                  │
│ - /                          │
│ - /api/cache/set             │
│ - /api/cache/get             │
└───────────────┬──────────────┘
                │
                ▼
┌──────────────────────────────┐
│ Mini Redis Hash Map          │
│ - hash_map_set               │
│ - hash_map_get               │
└──────────────────────────────┘
```

---

## 2. 功能特色

### 2.1 純 C 零依賴

程式只使用 Linux / POSIX 常見系統呼叫與標準 C library：

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>
```

沒有使用：

- libevent
- libev
- libuv
- nginx module
- Redis client library
- JSON-C
- cJSON
- mongoose
- civetweb

---

## 3. 第一區：自製 Hash Map 快取核心

### 3.1 資料結構

```c
typedef struct HashNode {
    char *key;
    char *value;
    struct HashNode *next;
} HashNode;

typedef struct {
    HashNode *buckets[HASH_MAP_SIZE];
} HashMap;
```

這是一個典型的 Hash Table + Linked List chaining 設計。

當兩個 key 算出相同 hash index 時，會透過 linked list 串接。

---

### 3.2 Hash 函式

```c
unsigned long djb2_hash(const char *str)
```

程式使用 `djb2` hash 演算法：

```c
hash = hash * 33 + c;
```

最後再取餘數：

```c
return hash % HASH_MAP_SIZE;
```

因此 key 會被映射到：

```c
0 ~ HASH_MAP_SIZE - 1
```

---

### 3.3 寫入快取

```c
void hash_map_set(HashMap *map, const char *key, const char *value)
```

行為：

1. 使用 `djb2_hash()` 計算 bucket index
2. 檢查該 bucket linked list 中是否已有相同 key
3. 如果 key 已存在：
   - 釋放舊 value
   - 使用 `strdup()` 複製新 value
4. 如果 key 不存在：
   - 建立新的 `HashNode`
   - 插入 bucket linked list 開頭

---

### 3.4 讀取快取

```c
const char* hash_map_get(HashMap *map, const char *key)
```

行為：

1. 使用 key 計算 bucket index
2. 掃描該 bucket linked list
3. 找到相同 key 就回傳 value
4. 找不到就回傳 `NULL`

平均情況下接近 O(1)，但如果 hash collision 很多，最壞情況會退化成 O(n)。

---

## 4. 第二區：Mini Express 框架層

### 4.1 Request / Response

```c
typedef struct {
    const char *method;
    const char *path;
    const char *body;
} Request;

typedef struct {
    int status_code;
    char response_buf[4096];
} Response;
```

`Request` 用來保存 HTTP request 的基本資訊：

- method，例如 GET / POST
- path，例如 `/api/cache/set`
- body，例如 JSON 字串

`Response` 用來保存：

- HTTP status code
- JSON response body

---

### 4.2 Route Handler

```c
typedef void (*RouteHandler)(Request *, Response *);
```

每個 API handler 都符合這個格式：

```c
void handler(Request *req, Response *res)
```

---

### 4.3 路由表

```c
typedef struct {
    const char *method;
    const char *path;
    RouteHandler handler;
} RouteRule;
```

`lib/core/mini_express.c` 內部維護 route table：

```c
static RouteRule route_table[MAX_ROUTES];
static int route_count;
```

`lib/core/mini_express.h` 對外提供路由註冊 API：

```c
void app_get(const char *path, RouteHandler handler);
void app_post(const char *path, RouteHandler handler);
```

使用方式：

```c
app_get("/", handle_home);
app_post("/api/cache/set", handle_set_cache);
app_get("/api/cache/get", handle_get_cache);
```

---

## 5. JSON 欄位提取器

```c
int json_get_string(const char *json, const char *field, char *output, size_t max_len)
```

這是一個非常簡化的 JSON parser，只支援以下格式：

```json
{
  "key": "name",
  "value": "Nick"
}
```

它會尋找：

```text
"field" : "value"
```

例如：

```c
json_get_string(req->body, "key", key, sizeof(key));
json_get_string(req->body, "value", value, sizeof(value));
```

### 限制

這個 parser 不支援：

- escape 字元，例如 `\"`
- nested object
- array
- number / boolean / null
- Unicode escape
- JSON 格式完整驗證

因此它適合教學或簡單 demo，不適合直接用於 production。

---

## 6. API Handler

### 6.1 首頁 API

```c
void handle_home(Request *req, Response *res)
```

路由：

```http
GET /
```

回應：

```json
{
  "status": "success",
  "message": "Welcome to 100% Pure C Dependency-Free Express Engine!"
}
```

---

### 6.2 寫入快取 API

```c
void handle_set_cache(Request *req, Response *res)
```

路由：

```http
POST /api/cache/set
```

Request body：

```json
{
  "key": "name",
  "value": "Nick"
}
```

成功回應：

```json
{
  "status": "success",
  "message": "Saved to Custom O(1) Cache completely dependency-free!"
}
```

失敗回應：

```json
{
  "status": "error",
  "error": "Missing or invalid JSON fields: 'key' or 'value'"
}
```

---

### 6.3 讀取快取 API

```c
void handle_get_cache(Request *req, Response *res)
```

路由：

```http
GET /api/cache/get
```

Request body：

```json
{
  "key": "name"
}
```

成功回應：

```json
{
  "status": "hit",
  "key": "name",
  "value": "Nick"
}
```

找不到 key：

```json
{
  "status": "error",
  "error": "Key not found in local hashmap database"
}
```

---

## 7. epoll 網路核心

### 7.1 ClientState

```c
typedef struct {
    int fd;
    char read_buf[4096];
    int read_idx;
} ClientState;
```

每個 client connection 都有自己的狀態：

- socket fd
- read buffer
- 目前已讀取位置

---

### 7.2 設定 Non-blocking

```c
void set_nonblocking(int fd)
{
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
}
```

這讓 socket 在沒有資料可讀時不會阻塞，而是回傳 `EAGAIN` 或 `EWOULDBLOCK`。

---

### 7.3 epoll event loop

核心流程：

```text
epoll_wait()
   │
   ├── server fd readable
   │      └── accept new client
   │
   └── client fd readable
          ├── read request
          ├── parse method/path/body
          ├── dispatch route handler
          ├── write response
          └── close connection
```

---

## 8. HTTP Request 分析

程式使用：

```c
sscanf(state->read_buf, "%15s %255s", method, path);
```

從 HTTP request 第一行取得 method 和 path。

例如：

```http
POST /api/cache/set HTTP/1.1
```

會得到：

```text
method = POST
path   = /api/cache/set
```

HTTP body 則透過：

```c
char *body = strstr(state->read_buf, "\r\n\r\n");
if (body) body += 4;
```

找到 header/body 的分隔位置。

---

## 9. 編譯方式

專案目前拆成：

```bash
main.c
lib/core/mini_express.c
lib/core/mini_express.h
lib/core-net/epoll_server.c
lib/core-net/epoll_server.h
lib/misc/hash_map.c
lib/misc/hash_map.h
lib/misc/json_parser.c
lib/misc/json_parser.h
lib/misc/static_files.c
lib/misc/static_files.h
lib/roles/roles.c
lib/roles/roles.h
lib/roles/http1.c
lib/roles/http1.h
lib/roles/http2.c
lib/roles/http2.h
lib/roles/http3.c
lib/roles/http3.h
lib/roles/ws.c
lib/roles/ws.h
public/index.html
public/ws-test.html
public/ws-test.js
tests/ws_handshake_test.js
```

使用 Makefile 編譯：

```bash
make
```

Node.js WebSocket handshake 測試：

```bash
make test-js
```

目前 `main.c` 由 Makefile 以 C++ 模式編譯，支援 non-capturing lambda handler；`lib/` 內的其他模組仍以 C 編譯。

```bash
c++ -Wall -Wextra -O2 -x c++ -c main.c -o main.o
```

---

## 10. 執行方式

```bash
./mini_express
```

預設監聽 `8080`，所以首頁靜態頁面網址是：

```bash
http://127.0.0.1:8080/
```

如果要使用不帶 port 的網址：

```bash
http://127.0.0.1/
```

需要監聽 port 80：

```bash
sudo ./mini_express 80
```

成功啟動後會看到：

```text
100% 純 C 零依賴自製 Web 引擎全面運作中！Port 8080...
```

預設監聽 port：

```c
#define PORT 8080
```

---

## 11. 測試方式

### 11.1 測試首頁

```bash
curl http://127.0.0.1:8080/
```

---

### 11.2 寫入快取

```bash
curl -X POST http://127.0.0.1:8080/api/cache/set \
  -H "Content-Type: application/json" \
  -d '{"key":"name","value":"Nick"}'
```

---

### 11.3 讀取快取

因為目前程式把 `/api/cache/get` 設計成 `GET`，但仍然從 body 讀取 JSON，所以可以這樣測試：

```bash
curl -X GET http://127.0.0.1:8080/api/cache/get \
  -H "Content-Type: application/json" \
  -d '{"key":"name"}'
```

回應範例：

```json
{
  "status": "hit",
  "key": "name",
  "value": "Nick"
}
```

---

## 12. 目前程式限制

### 12.1 HTTP parser 非完整實作

目前只簡單解析：

- method
- path
- body 起始位置

沒有完整處理：

- Content-Length
- chunked transfer encoding
- keep-alive
- header 欄位
- URL query string
- multipart/form-data

---

### 12.2 GET API 使用 body 不太標準

目前 `/api/cache/get` 是：

```c
app_get("/api/cache/get", handle_get_cache);
```

但 handler 會從 request body 讀取 key。

更常見的 REST 設計會是：

```http
GET /api/cache/get?key=name
```

或改成：

```http
POST /api/cache/get
```

---

### 12.3 write() 可能沒有完整送出

目前回應使用：

```c
write(client->fd, header, strlen(header));
write(client->fd, res.response_buf, strlen(res.response_buf));
```

在 non-blocking socket 下，`write()` 可能只送出部分資料。

production 寫法應該要：

- 檢查 `write()` 回傳值
- 尚未送完的資料放入 write buffer
- 註冊 `EPOLLOUT`
- 等 socket 可寫時繼續送

---

### 12.4 Edge Trigger accept 應該用迴圈

目前 server fd 使用：

```c
ev.events = EPOLLIN;
```

client fd 使用：

```c
ev.events = EPOLLIN | EPOLLET;
```

如果 server fd 也改成 `EPOLLET`，accept 必須寫成迴圈：

```c
while ((client_fd = accept(server_fd, NULL, NULL)) != -1) {
    ...
}
```

---

### 12.5 記憶體釋放不完整

HashMap 內的 key/value 使用 `strdup()` 配置：

```c
new_node->key = strdup(key);
new_node->value = strdup(value);
```

程式結束前沒有釋放整個 HashMap。

雖然 server 通常長時間執行，但如果要做乾淨釋放，應該補上：

```c
void hash_map_free(HashMap *map)
```

---

### 12.6 JSON response 沒有 escape

如果 value 中包含：

```text
"
\
換行
```

可能會產生不合法 JSON。

正式環境應該要實作 JSON string escaping。

---

## 13. 建議改進方向

可以逐步加入以下功能：

1. 支援 query string，例如 `/api/cache/get?key=name`
2. 支援完整 `Content-Length` 讀取
3. 支援 `EPOLLOUT` 非阻塞寫入
4. 加入 `hash_map_delete()`
5. 加入 `hash_map_free()`
6. 加入 JSON escape
7. 加入 timeout 機制
8. 加入 logging
9. 加入 route parameter，例如 `/api/cache/:key`
10. 加入 thread pool 或 worker model

---

## 14. 總結

這份程式是一個很適合學習 Linux network programming 的純 C Web server 範例。

它展示了：

- 如何使用 socket 建立 TCP server
- 如何使用 epoll 做事件驅動
- 如何建立簡易 route table
- 如何設計 Request / Response abstraction
- 如何用 Hash Map 做記憶體快取
- 如何從 HTTP body 中提取簡單 JSON 欄位

雖然它不適合直接用於 production，但非常適合作為理解 Web framework、HTTP server、event loop、cache system 的學習基礎。
