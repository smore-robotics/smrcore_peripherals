/**
 * @file webxr_server.cpp
 * @brief Minimal localhost HTTP/WebSocket server for PICO WebXR.
 */

#include "protocols/pico/webxr_server.hpp"

#include "common/clock.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <cerrno>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string_view>
#include <utility>
#include <vector>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

namespace smrcore::peripherals::protocols::pico
{
namespace
{

constexpr char kWebSocketMagic[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
constexpr size_t kMaxHttpRequestBytes = 16 * 1024;
constexpr size_t kMaxWebSocketPayloadBytes = 64 * 1024;

constexpr char kWebPage[] = R"HTML(<!doctype html>
<html lang="zh-CN">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <title>PICO WebXR 外设输入</title>
  <style>
    body { margin: 0; padding: 32px; color: #eef6ff; background: #071526;
           font: 22px system-ui, sans-serif; }
    main { max-width: 900px; margin: auto; }
    button { font-size: 24px; padding: 18px 28px; border: 0; border-radius: 14px;
             color: white; background: #1479e8; }
    button:disabled { opacity: .45; }
    .ok { color: #62e6a7; } .bad { color: #ff8f8f; }
    pre { padding: 18px; overflow: auto; border-radius: 12px;
          background: #0e2239; font-size: 16px; }
    canvas { width: 1px; height: 1px; }
  </style>
</head>
<body>
<main>
  <h1>PICO WebXR 外设输入</h1>
  <p id="xr" class="bad">WebXR：检查中</p>
  <p id="ws" class="bad">PC连接：连接中</p>
  <p>此页面只向电脑发送头显、左右手柄位姿以及 Grip/Trigger 数值，
     不连接机器人，也不产生运动命令。</p>
  <button id="enter" disabled>检查 WebXR</button>
  <pre id="pose">等待进入 VR…</pre>
  <canvas id="canvas"></canvas>
</main>
<script>
const enter = document.querySelector("#enter");
const xr = document.querySelector("#xr");
const ws = document.querySelector("#ws");
const output = document.querySelector("#pose");
const canvas = document.querySelector("#canvas");
let socket, session, ref, refName = "", gl, seq = 0, lastSend = 0;

function status(el, text, ok) {
  el.textContent = text;
  el.className = ok ? "ok" : "bad";
}
function connect() {
  socket = new WebSocket(`ws://${location.host}/ws`);
  socket.onopen = () => status(ws, "PC连接：已连接", true);
  socket.onclose = () => {
    status(ws, "PC连接：已断开，正在重连", false);
    setTimeout(connect, 1000);
  };
  socket.onerror = () => status(ws, "PC连接：错误", false);
}
function round(v) { return Number(v.toFixed(6)); }
function controller(frame, source) {
  if (!source.gripSpace) return null;
  const pose = frame.getPose(source.gripSpace, ref);
  if (!pose) return { handedness: source.handedness, tracked: false };
  const p = pose.transform.position;
  const q = pose.transform.orientation;
  const buttons = source.gamepad ? source.gamepad.buttons : [];
  const axes = source.gamepad?.axes || [];
  // xr-standard reserves axes[0:2] for the touchpad and axes[2:4] for
  // the primary thumbstick. Some PICO browser builds expose the pair in
  // the legacy [0:2] layout, so choose the pair with actual motion.
  let thumbstickOffset = 0;
  if (axes.length >= 4) {
    const standardMagnitude = Math.abs(axes[2] || 0) + Math.abs(axes[3] || 0);
    const legacyMagnitude = Math.abs(axes[0] || 0) + Math.abs(axes[1] || 0);
    thumbstickOffset = standardMagnitude >= legacyMagnitude ? 2 : 0;
  }
  return {
    handedness: source.handedness,
    tracked: true,
    position: [round(p.x), round(p.y), round(p.z)],
    orientation_xyzw: [round(q.x), round(q.y), round(q.z), round(q.w)],
    trigger: round(buttons[0]?.value || 0),
    grip: round(buttons[1]?.value || 0),
    thumbstick: [round(axes[thumbstickOffset] || 0),
                 round(axes[thumbstickOffset + 1] || 0)]
  };
}
function frame(time, f) {
  f.session.requestAnimationFrame(frame);
  const viewer = f.getViewerPose(ref);
  let headset = {tracked: false};
  if (viewer) {
    const p = viewer.transform.position;
    const q = viewer.transform.orientation;
    headset = {
      tracked: true,
      position: [round(p.x), round(p.y), round(p.z)],
      orientation_xyzw: [round(q.x), round(q.y), round(q.z), round(q.w)]
    };
    const layer = f.session.renderState.baseLayer;
    gl.bindFramebuffer(gl.FRAMEBUFFER, layer.framebuffer);
    for (const view of viewer.views) {
      const vp = layer.getViewport(view);
      gl.viewport(vp.x, vp.y, vp.width, vp.height);
      gl.clearColor(.025, .07, .12, 1);
      gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);
    }
  }
  if (time - lastSend < 20) return;
  lastSend = time;
  const controllers = {};
  for (const source of f.session.inputSources) {
    const value = controller(f, source);
    if (value && (value.handedness === "left" ||
                  value.handedness === "right")) {
      controllers[value.handedness] = value;
    }
  }
  const packet = {
    type: "pico_controller_pose", sequence: seq++,
    timestamp_ms: round(performance.now()),
    reference_space: refName, headset, controllers
  };
  output.textContent = JSON.stringify(packet, null, 2);
  if (socket?.readyState === WebSocket.OPEN &&
      socket.bufferedAmount < 65536) {
    socket.send(JSON.stringify(packet));
  }
}
async function start() {
  enter.disabled = true;
  try {
    session = await navigator.xr.requestSession("immersive-vr");
    gl = canvas.getContext("webgl", {xrCompatible: true, alpha: false});
    await gl.makeXRCompatible();
    session.updateRenderState({baseLayer: new XRWebGLLayer(session, gl)});
    try {
      ref = await session.requestReferenceSpace("local-floor");
      refName = "local-floor";
    } catch (_) {
      ref = await session.requestReferenceSpace("local");
      refName = "local";
    }
    session.onend = () => {
      enter.disabled = false;
      enter.textContent = "重新进入 VR";
      status(xr, "WebXR：会话已结束", false);
    };
    status(xr, `WebXR：运行中（${refName}）`, true);
    session.requestAnimationFrame(frame);
  } catch (e) {
    enter.disabled = false;
    status(xr, `WebXR启动失败：${e.message}`, false);
  }
}
async function init() {
  connect();
  if (!navigator.xr ||
      !await navigator.xr.isSessionSupported("immersive-vr")) {
    status(xr, "WebXR：当前浏览器不支持沉浸式 VR", false);
    return;
  }
  status(xr, "WebXR：PICO运行时可用", true);
  enter.disabled = false;
  enter.textContent = "进入 VR 并开始追踪";
}
enter.onclick = start;
init().catch(e => status(xr, `初始化失败：${e.message}`, false));
</script>
</body>
</html>)HTML";

uint32_t RotateLeft(uint32_t value, unsigned bits)
{
    return (value << bits) | (value >> (32U - bits));
}

std::array<uint8_t, 20> Sha1(std::string_view input)
{
    std::vector<uint8_t> data(input.begin(), input.end());
    const uint64_t bit_count = static_cast<uint64_t>(data.size()) * 8U;
    data.push_back(0x80);
    while ((data.size() % 64U) != 56U)
    {
        data.push_back(0);
    }
    for (int shift = 56; shift >= 0; shift -= 8)
    {
        data.push_back(static_cast<uint8_t>(bit_count >> shift));
    }

    uint32_t h0 = 0x67452301U;
    uint32_t h1 = 0xEFCDAB89U;
    uint32_t h2 = 0x98BADCFEU;
    uint32_t h3 = 0x10325476U;
    uint32_t h4 = 0xC3D2E1F0U;
    for (size_t block = 0; block < data.size(); block += 64)
    {
        std::array<uint32_t, 80> words = {};
        for (size_t i = 0; i < 16; ++i)
        {
            const size_t offset = block + i * 4;
            words[i] = (static_cast<uint32_t>(data[offset]) << 24U) |
                       (static_cast<uint32_t>(data[offset + 1]) << 16U) |
                       (static_cast<uint32_t>(data[offset + 2]) << 8U) |
                       static_cast<uint32_t>(data[offset + 3]);
        }
        for (size_t i = 16; i < words.size(); ++i)
        {
            words[i] = RotateLeft(
                words[i - 3] ^ words[i - 8] ^ words[i - 14] ^ words[i - 16], 1);
        }

        uint32_t a = h0;
        uint32_t b = h1;
        uint32_t c = h2;
        uint32_t d = h3;
        uint32_t e = h4;
        for (size_t i = 0; i < words.size(); ++i)
        {
            uint32_t f = 0;
            uint32_t k = 0;
            if (i < 20)
            {
                f = (b & c) | ((~b) & d);
                k = 0x5A827999U;
            }
            else if (i < 40)
            {
                f = b ^ c ^ d;
                k = 0x6ED9EBA1U;
            }
            else if (i < 60)
            {
                f = (b & c) | (b & d) | (c & d);
                k = 0x8F1BBCDCU;
            }
            else
            {
                f = b ^ c ^ d;
                k = 0xCA62C1D6U;
            }
            const uint32_t temp = RotateLeft(a, 5) + f + e + k + words[i];
            e = d;
            d = c;
            c = RotateLeft(b, 30);
            b = a;
            a = temp;
        }
        h0 += a;
        h1 += b;
        h2 += c;
        h3 += d;
        h4 += e;
    }

    std::array<uint8_t, 20> digest = {};
    const std::array<uint32_t, 5> hash = {h0, h1, h2, h3, h4};
    for (size_t i = 0; i < hash.size(); ++i)
    {
        digest[i * 4] = static_cast<uint8_t>(hash[i] >> 24U);
        digest[i * 4 + 1] = static_cast<uint8_t>(hash[i] >> 16U);
        digest[i * 4 + 2] = static_cast<uint8_t>(hash[i] >> 8U);
        digest[i * 4 + 3] = static_cast<uint8_t>(hash[i]);
    }
    return digest;
}

std::string Base64(const uint8_t *data, size_t size)
{
    constexpr char alphabet[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string result;
    result.reserve(((size + 2) / 3) * 4);
    for (size_t i = 0; i < size; i += 3)
    {
        const uint32_t value =
            (static_cast<uint32_t>(data[i]) << 16U) |
            (i + 1 < size ? static_cast<uint32_t>(data[i + 1]) << 8U : 0U) |
            (i + 2 < size ? static_cast<uint32_t>(data[i + 2]) : 0U);
        result.push_back(alphabet[(value >> 18U) & 0x3FU]);
        result.push_back(alphabet[(value >> 12U) & 0x3FU]);
        result.push_back(i + 1 < size ? alphabet[(value >> 6U) & 0x3FU] : '=');
        result.push_back(i + 2 < size ? alphabet[value & 0x3FU] : '=');
    }
    return result;
}

bool SendAll(int fd, const uint8_t *data, size_t size)
{
    size_t sent = 0;
    while (sent < size)
    {
        const ssize_t count =
            ::send(fd, data + sent, size - sent, MSG_NOSIGNAL);
        if (count <= 0)
        {
            return false;
        }
        sent += static_cast<size_t>(count);
    }
    return true;
}

bool SendText(int fd, const std::string &text)
{
    return SendAll(fd, reinterpret_cast<const uint8_t *>(text.data()),
                   text.size());
}

std::string HeaderValue(const std::string &request, const std::string &name)
{
    const std::string needle = "\r\n" + name + ":";
    const size_t found = request.find(needle);
    if (found == std::string::npos)
    {
        return {};
    }
    size_t begin = found + needle.size();
    while (begin < request.size() && request[begin] == ' ')
    {
        ++begin;
    }
    const size_t end = request.find("\r\n", begin);
    return request.substr(begin, end - begin);
}

bool ReadHttpRequest(int fd, std::string &request)
{
    std::array<char, 2048> buffer = {};
    while (request.find("\r\n\r\n") == std::string::npos &&
           request.size() < kMaxHttpRequestBytes)
    {
        const ssize_t count = ::recv(fd, buffer.data(), buffer.size(), 0);
        if (count <= 0)
        {
            return false;
        }
        request.append(buffer.data(), static_cast<size_t>(count));
    }
    return request.find("\r\n\r\n") != std::string::npos;
}

bool ReadExact(int fd, uint8_t *data, size_t size)
{
    size_t received = 0;
    while (received < size)
    {
        const ssize_t count = ::recv(fd, data + received, size - received, 0);
        if (count <= 0)
        {
            return false;
        }
        received += static_cast<size_t>(count);
    }
    return true;
}

bool ReadWebSocketFrame(int fd, uint8_t &opcode, std::string &payload)
{
    std::array<uint8_t, 2> header = {};
    if (!ReadExact(fd, header.data(), header.size()))
    {
        return false;
    }
    opcode = header[0] & 0x0FU;
    uint64_t length = header[1] & 0x7FU;
    if (length == 126)
    {
        std::array<uint8_t, 2> extended = {};
        if (!ReadExact(fd, extended.data(), extended.size()))
        {
            return false;
        }
        length = (static_cast<uint64_t>(extended[0]) << 8U) | extended[1];
    }
    else if (length == 127)
    {
        std::array<uint8_t, 8> extended = {};
        if (!ReadExact(fd, extended.data(), extended.size()))
        {
            return false;
        }
        length = 0;
        for (uint8_t byte : extended)
        {
            length = (length << 8U) | byte;
        }
    }
    if (length > kMaxWebSocketPayloadBytes)
    {
        return false;
    }

    const bool masked = (header[1] & 0x80U) != 0;
    std::array<uint8_t, 4> mask = {};
    if (masked && !ReadExact(fd, mask.data(), mask.size()))
    {
        return false;
    }
    payload.resize(static_cast<size_t>(length));
    if (length > 0 &&
        !ReadExact(fd, reinterpret_cast<uint8_t *>(payload.data()),
                   payload.size()))
    {
        return false;
    }
    if (masked)
    {
        for (size_t i = 0; i < payload.size(); ++i)
        {
            payload[i] = static_cast<char>(static_cast<uint8_t>(payload[i]) ^
                                           mask[i % 4]);
        }
    }
    return true;
}

void SendControlFrame(int fd, uint8_t opcode, const std::string &payload)
{
    if (payload.size() >= 126)
    {
        return;
    }
    std::vector<uint8_t> frame = {
        static_cast<uint8_t>(0x80U | opcode),
        static_cast<uint8_t>(payload.size()),
    };
    frame.insert(frame.end(), payload.begin(), payload.end());
    SendAll(fd, frame.data(), frame.size());
}

bool ReadFiniteArray(const nlohmann::json &value, size_t size, double *output)
{
    if (!value.is_array() || value.size() != size)
    {
        return false;
    }
    for (size_t i = 0; i < size; ++i)
    {
        if (!value[i].is_number())
        {
            return false;
        }
        output[i] = value[i].get<double>();
        if (!std::isfinite(output[i]))
        {
            return false;
        }
    }
    return true;
}

PicoControllerPose ParseController(const nlohmann::json &controllers,
                                   const char *name)
{
    PicoControllerPose result;
    const auto found = controllers.find(name);
    if (found == controllers.end() || !found->is_object())
    {
        return result;
    }
    result.present = true;
    result.tracked = found->value("tracked", false);
    result.trigger = std::clamp(found->value("trigger", 0.0), 0.0, 1.0);
    result.grip = std::clamp(found->value("grip", 0.0), 0.0, 1.0);
    const auto buttons = found->find("buttons");
    if (buttons != found->end() && buttons->is_array())
    {
        if (buttons->size() > 0 && (*buttons)[0].is_object())
        {
    result.trigger = std::clamp(
                (*buttons)[0].value("value", result.trigger), 0.0, 1.0);
        }
        if (buttons->size() > 1 && (*buttons)[1].is_object())
        {
            result.grip =
                std::clamp((*buttons)[1].value("value", result.grip), 0.0, 1.0);
        }
    }
    const auto thumbstick = found->find("thumbstick");
    if (thumbstick != found->end() &&
        !ReadFiniteArray(*thumbstick, 2, result.thumbstick.data()))
    {
        result.tracked = false;
    }
    if (!std::isfinite(result.trigger) || !std::isfinite(result.grip) ||
        !std::isfinite(result.thumbstick[0]) ||
        !std::isfinite(result.thumbstick[1]) ||
        !ReadFiniteArray(found->value("position", nlohmann::json::array()), 3,
                         result.position.data()) ||
        !ReadFiniteArray(
            found->value("orientation_xyzw", nlohmann::json::array()), 4,
            result.orientation_xyzw.data()))
    {
        result.tracked = false;
    }
    return result;
}

PicoHeadsetPose ParseHeadset(const nlohmann::json &value)
{
    PicoHeadsetPose result;
    if (!value.is_object())
    {
        return result;
    }
    result.present = true;
    result.tracked = value.value("tracked", false);
    if (!ReadFiniteArray(value.value("position", nlohmann::json::array()), 3,
                         result.position.data()) ||
        !ReadFiniteArray(
            value.value("orientation_xyzw", nlohmann::json::array()), 4,
            result.orientation_xyzw.data()))
    {
        result.tracked = false;
    }
    return result;
}

} // namespace

WebXrServer::~WebXrServer() { Stop(); }

bool WebXrServer::Start(uint16_t port, std::string &error)
{
    if (running_.exchange(true))
    {
        error = "server is already running";
        return false;
    }
    client_connected_ = false;
    {
        std::lock_guard<std::mutex> lock(packet_mutex_);
        latest_packet_.reset();
    }

    const int fd = ::socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (fd < 0)
    {
        running_ = false;
        error = std::strerror(errno);
        return false;
    }
    int reuse = 1;
    ::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    sockaddr_in address = {};
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (::bind(fd, reinterpret_cast<sockaddr *>(&address), sizeof(address)) !=
            0 ||
        ::listen(fd, 4) != 0)
    {
        error = std::strerror(errno);
        ::close(fd);
        running_ = false;
        return false;
    }
    listen_fd_ = fd;
    thread_ = std::thread(&WebXrServer::Run, this);
    return true;
}

void WebXrServer::Stop()
{
    if (!running_.exchange(false))
    {
        return;
    }
    const int client = client_fd_.load();
    if (client >= 0)
    {
        ::shutdown(client, SHUT_RDWR);
    }
    const int listener = listen_fd_.exchange(-1);
    if (listener >= 0)
    {
        ::shutdown(listener, SHUT_RDWR);
        ::close(listener);
    }
    if (thread_.joinable())
    {
        thread_.join();
    }
    const int remaining_client = client_fd_.exchange(-1);
    if (remaining_client >= 0)
    {
        ::close(remaining_client);
    }
}

bool WebXrServer::IsRunning() const { return running_.load(); }

bool WebXrServer::IsClientConnected() const { return client_connected_.load(); }

std::optional<PicoSample> WebXrServer::TakeSample()
{
    std::lock_guard<std::mutex> lock(packet_mutex_);
    auto packet = std::move(latest_packet_);
    latest_packet_.reset();
    return packet;
}

void WebXrServer::Run()
{
    while (running_)
    {
        pollfd descriptor = {listen_fd_.load(), POLLIN, 0};
        const int ready = ::poll(&descriptor, 1, 100);
        if (ready <= 0 || (descriptor.revents & POLLIN) == 0)
        {
            continue;
        }
        const int client =
            ::accept4(descriptor.fd, nullptr, nullptr, SOCK_CLOEXEC);
        if (client < 0)
        {
            continue;
        }
        // Chromium may open an idle speculative connection before the real
        // page/WebSocket request. Do not let it monopolize this single-client
        // accept loop.
        timeval handshake_timeout = {2, 0};
        ::setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, &handshake_timeout,
                     sizeof(handshake_timeout));
        client_fd_ = client;
        HandleClient(client);
        client_connected_ = false;
        const int owned_client = client_fd_.exchange(-1);
        if (owned_client >= 0)
        {
            ::close(owned_client);
        }
    }
}

void WebXrServer::HandleClient(int client_fd)
{
    std::string request;
    if (!ReadHttpRequest(client_fd, request))
    {
        return;
    }
    const bool websocket =
        request.rfind("GET /ws ", 0) == 0 &&
        request.find("\r\nUpgrade: websocket") != std::string::npos;
    if (!websocket)
    {
        if (request.rfind("GET / ", 0) != 0)
        {
            SendText(client_fd,
                     "HTTP/1.1 404 Not Found\r\nConnection: close\r\n"
                     "Content-Length: 0\r\n\r\n");
            return;
        }
        std::ostringstream response;
        response << "HTTP/1.1 200 OK\r\n"
                 << "Content-Type: text/html; charset=utf-8\r\n"
                 << "Cache-Control: no-store\r\nConnection: close\r\n"
                 << "Content-Length: " << std::strlen(kWebPage) << "\r\n\r\n"
                 << kWebPage;
        SendText(client_fd, response.str());
        return;
    }

    const std::string key = HeaderValue(request, "Sec-WebSocket-Key");
    if (key.empty())
    {
        return;
    }
    const auto digest = Sha1(key + kWebSocketMagic);
    const std::string accept = Base64(digest.data(), digest.size());
    if (!SendText(client_fd, "HTTP/1.1 101 Switching Protocols\r\n"
                             "Upgrade: websocket\r\nConnection: Upgrade\r\n"
                             "Sec-WebSocket-Accept: " +
                                 accept + "\r\n\r\n"))
    {
        return;
    }
    client_connected_ = true;
    timeval no_timeout = {0, 0};
    ::setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &no_timeout,
                 sizeof(no_timeout));

    while (running_)
    {
        uint8_t opcode = 0;
        std::string payload;
        if (!ReadWebSocketFrame(client_fd, opcode, payload))
        {
            return;
        }
        if (opcode == 0x8)
        {
            SendControlFrame(client_fd, 0x8, {});
            return;
        }
        if (opcode == 0x9)
        {
            SendControlFrame(client_fd, 0xA, payload);
            continue;
        }
        if (opcode != 0x1)
        {
            continue;
        }
        PicoSample packet;
        if (!ParsePacket(payload, packet))
        {
            continue;
        }
        std::lock_guard<std::mutex> lock(packet_mutex_);
        latest_packet_ = std::move(packet);
    }
}

bool WebXrServer::ParsePacket(const std::string &text, PicoSample &packet) const
{
    const nlohmann::json json =
        nlohmann::json::parse(text, nullptr, false, true);
    if (json.is_discarded() || !json.is_object() ||
        json.value("type", "") != "pico_controller_pose")
    {
        return false;
    }
    packet.sequence = json.value("sequence", uint64_t{0});
    packet.headset_timestamp_ms = json.value("timestamp_ms", 0.0);
    packet.reference_space = json.value("reference_space", "");
    packet.timestamp_sec = internal::NowSeconds();
    packet.headset =
        ParseHeadset(json.value("headset", nlohmann::json::object()));
    const auto controllers =
        json.value("controllers", nlohmann::json::object());
    if (!controllers.is_object())
    {
        return false;
    }
    packet.left = ParseController(controllers, "left");
    packet.right = ParseController(controllers, "right");
    return std::isfinite(packet.headset_timestamp_ms);
}

} // namespace smrcore::peripherals::protocols::pico
