#include "game/License.hpp"

#include "platform/linux/LinuxServices.hpp"

#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace game {
namespace {

constexpr char serviceName[] = "com.dingoo.server.VerifyService";

uint16_t Read16BE(const uint8_t* data)
{
    return static_cast<uint16_t>((static_cast<uint16_t>(data[0]) << 8) | data[1]);
}

uint32_t Read32BE(const uint8_t* data)
{
    return (static_cast<uint32_t>(data[0]) << 24) | (static_cast<uint32_t>(data[1]) << 16) |
        (static_cast<uint32_t>(data[2]) << 8) | data[3];
}

uint32_t Read32LE(const uint8_t* data)
{
    return static_cast<uint32_t>(data[0]) | (static_cast<uint32_t>(data[1]) << 8) |
        (static_cast<uint32_t>(data[2]) << 16) | (static_cast<uint32_t>(data[3]) << 24);
}

void Append16BE(std::vector<uint8_t>& output, uint16_t value)
{
    output.push_back(static_cast<uint8_t>(value >> 8));
    output.push_back(static_cast<uint8_t>(value));
}

uint32_t RotateLeft(uint32_t value, unsigned bits)
{
    return (value << bits) | (value >> (32 - bits));
}

// DES block processing and the 512-word SP table originate from Eric Young's libdes
// (1992), as used by the native game. The native little-endian block layout and the
// decryptor's reverse schedule order are preserved here.
// Libdes copyright and license: Eric Young, 1992; see the libdes LICENSE notice at
// https://codebrowser.dev/glibc/glibc/sunrpc/des_impl.c.html
std::array<uint8_t, 8> CryptDesBlock(const uint8_t* input,
                                     const std::array<uint8_t, 128>& schedule, bool decrypt)
{
    static constexpr uint32_t sp[512] = {
#include "game/DesSpTable.inc"
    };
    const auto rotateRight = [](uint32_t value, unsigned bits) {
        return (value >> bits) | (value << (32 - bits));
    };
    const auto permute = [](uint32_t& a, uint32_t& b, unsigned shift, uint32_t mask) {
        const uint32_t mixed = ((a >> shift) ^ b) & mask;
        b ^= mixed;
        a ^= mixed << shift;
    };
    uint32_t right = Read32LE(input);
    uint32_t left = Read32LE(input + 4);
    permute(right, left, 4, 0x0f0f0f0f);
    permute(left, right, 16, 0x0000ffff);
    permute(right, left, 2, 0x33333333);
    permute(left, right, 8, 0x00ff00ff);
    permute(right, left, 1, 0x55555555);
    const uint32_t initialRight = RotateLeft(right, 1);
    right = RotateLeft(left, 1);
    left = initialRight;
    for (int round = 0; round < 16; ++round) {
        const int selected = decrypt ? 15 - round : round;
        const uint32_t first = Read32LE(schedule.data() + selected * 8);
        const uint32_t second = Read32LE(schedule.data() + selected * 8 + 4);
        const uint32_t even = right ^ first;
        const uint32_t odd = rotateRight(right ^ second, 4);
        const uint32_t roundValue = sp[even & 0x3f] ^ sp[64 + (odd & 0x3f)] ^
            sp[128 + ((even >> 8) & 0x3f)] ^ sp[192 + ((odd >> 8) & 0x3f)] ^
            sp[256 + ((even >> 16) & 0x3f)] ^ sp[320 + ((odd >> 16) & 0x3f)] ^
            sp[384 + ((even >> 24) & 0x3f)] ^ sp[448 + ((odd >> 24) & 0x3f)];
        const uint32_t next = left ^ roundValue;
        left = right;
        right = next;
    }
    left = rotateRight(left, 1);
    right = rotateRight(right, 1);
    permute(right, left, 1, 0x55555555);
    permute(left, right, 8, 0x00ff00ff);
    permute(right, left, 2, 0x33333333);
    permute(left, right, 16, 0x0000ffff);
    permute(right, left, 4, 0x0f0f0f0f);
    std::array<uint8_t, 8> output{};
    for (int index = 0; index < 4; ++index) {
        output[index] = static_cast<uint8_t>(right >> (index * 8));
        output[index + 4] = static_cast<uint8_t>(left >> (index * 8));
    }
    return output;
}

void Md5Transform(uint32_t state[4], const uint8_t block[64])
{
    static constexpr uint32_t constants[64] = {
        0xd76aa478,0xe8c7b756,0x242070db,0xc1bdceee,0xf57c0faf,0x4787c62a,0xa8304613,0xfd469501,
        0x698098d8,0x8b44f7af,0xffff5bb1,0x895cd7be,0x6b901122,0xfd987193,0xa679438e,0x49b40821,
        0xf61e2562,0xc040b340,0x265e5a51,0xe9b6c7aa,0xd62f105d,0x02441453,0xd8a1e681,0xe7d3fbc8,
        0x21e1cde6,0xc33707d6,0xf4d50d87,0x455a14ed,0xa9e3e905,0xfcefa3f8,0x676f02d9,0x8d2a4c8a,
        0xfffa3942,0x8771f681,0x6d9d6122,0xfde5380c,0xa4beea44,0x4bdecfa9,0xf6bb4b60,0xbebfbc70,
        0x289b7ec6,0xeaa127fa,0xd4ef3085,0x04881d05,0xd9d4d039,0xe6db99e5,0x1fa27cf8,0xc4ac5665,
        0xf4292244,0x432aff97,0xab9423a7,0xfc93a039,0x655b59c3,0x8f0ccc92,0xffeff47d,0x85845dd1,
        0x6fa87e4f,0xfe2ce6e0,0xa3014314,0x4e0811a1,0xf7537e82,0xbd3af235,0x2ad7d2bb,0xeb86d391,
    };
    static constexpr uint8_t shifts[64] = {
        7,12,17,22,7,12,17,22,7,12,17,22,7,12,17,22,
        5,9,14,20,5,9,14,20,5,9,14,20,5,9,14,20,
        4,11,16,23,4,11,16,23,4,11,16,23,4,11,16,23,
        6,10,15,21,6,10,15,21,6,10,15,21,6,10,15,21,
    };
    uint32_t words[16];
    for (int index = 0; index < 16; ++index) {
        words[index] = Read32LE(block + index * 4);
    }
    uint32_t a = state[0], b = state[1], c = state[2], d = state[3];
    for (uint32_t index = 0; index < 64; ++index) {
        uint32_t value, word;
        if (index < 16) {
            value = (b & c) | (~b & d);
            word = index;
        } else if (index < 32) {
            value = (d & b) | (~d & c);
            word = (5 * index + 1) & 15;
        } else if (index < 48) {
            value = b ^ c ^ d;
            word = (3 * index + 5) & 15;
        } else {
            value = c ^ (b | ~d);
            word = (7 * index) & 15;
        }
        const uint32_t previousD = d;
        d = c;
        c = b;
        b += RotateLeft(a + value + constants[index] + words[word], shifts[index]);
        a = previousD;
    }
    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
}

} // namespace

License::License()
    : identityProvider(std::make_shared<soft3d::platform::LinuxDeviceIdentityProvider>())
    , network(std::make_shared<soft3d::platform::UnsupportedNetworkSession>())
    , sms(std::make_shared<soft3d::platform::LinuxSmsSession>())
{
    ResetSession();
}

void License::SetServices(std::shared_ptr<soft3d::platform::DeviceIdentityProvider> identityValue,
                          std::shared_ptr<soft3d::platform::NetworkSession> networkValue,
                          std::shared_ptr<soft3d::platform::SmsSession> smsValue)
{
    if (identityValue) identityProvider = std::move(identityValue);
    if (networkValue) network = std::move(networkValue);
    if (smsValue) sms = std::move(smsValue);
}

void License::SetMessageData(const void* data, size_t size)
{
    if (data == nullptr || size != messages.size() * messages[0].size()) {
        throw std::invalid_argument("license message.sdt must contain exactly 512 bytes");
    }
    std::memcpy(messages.data(), data, size);
}

void License::SetServer(std::string host, uint16_t port)
{
    serverHost = std::move(host);
    serverPort = port;
}

void License::SetMenuResultHandler(std::function<void(int)> handler)
{
    menuResult = std::move(handler);
}

void License::ResetSession()
{
    network->Cancel();
    sessionKeyReceived = false;
    sessionKeySchedule.fill(0);
    initialKeySchedule.fill(0);
    static constexpr char initial[16] = {'d','i','n','g','o','o','g','a','m','e',15,'7','d','a','y','s'};
    std::copy(std::begin(initial), std::end(initial), initialKeySchedule.begin());
}

bool License::ConnectSession()
{
    ResetSession();
    std::string identityText;
    if (!identityProvider->GetDeviceIdentity(identityText) || identityText.size() >= identity.size() ||
        !network->Connect(serverHost, serverPort)) {
        return false;
    }
    identity.fill(0);
    std::copy(identityText.begin(), identityText.end(), identity.begin());
    std::vector<uint8_t> plain(2 + 64 + 1 + 64, 0);
    std::memcpy(plain.data() + 2, serviceName, sizeof(serviceName) - 1);
    std::memcpy(plain.data() + 67, serviceName, sizeof(serviceName) - 1);
    return SendPlain(std::move(plain), true);
}

void License::CancelSession()
{
    network->Cancel();
    sms->Cancel();
}

bool License::PollSession()
{
    std::vector<uint8_t> response;
    const auto status = network->Poll(response);
    if (status == soft3d::platform::NetworkSession::Status::Failed) {
        return false;
    }
    return status == soft3d::platform::NetworkSession::Status::Pending ||
        HandleResponse(response.data(), response.size());
}

bool License::SendVerification(uint16_t operation, const std::string& text)
{
    if (!sessionKeyReceived) {
        return false;
    }
    const auto textDigest = Md5(text.data(), text.size());
    const auto identityDigest = Md5(identity.data(), identity.size());
    std::vector<uint8_t> plain;
    Append16BE(plain, 2);
    Append16BE(plain, operation);
    plain.insert(plain.end(), textDigest.begin(), textDigest.end());
    plain.insert(plain.end(), identityDigest.begin(), identityDigest.end());
    return SendPlain(std::move(plain), false);
}

bool License::SendPlain(std::vector<uint8_t> plain, bool initialKey)
{
    if (plain.size() < 2 || plain.size() > 0xffff) {
        return false;
    }
    return network->Send(EncodePacket(plain, initialKey ? initialKeySchedule : sessionKeySchedule));
}

std::vector<uint8_t> License::EncodePacket(const std::vector<uint8_t>& plain,
                                            const std::array<uint8_t, 128>& schedule)
{
    if (plain.size() < 2 || plain.size() > 0xffff) {
        return {};
    }
    std::vector<uint8_t> body(plain.begin() + 2, plain.end());
    body.resize((body.size() + 7) & ~size_t(7), 0);
    body = EncryptBlocks(body.data(), body.size(), schedule, false);
    std::vector<uint8_t> packet;
    Append16BE(packet, static_cast<uint16_t>(plain.size() + body.size() - (plain.size() - 2)));
    packet.insert(packet.end(), plain.begin(), plain.begin() + 2);
    packet.insert(packet.end(), body.begin(), body.end());
    return packet;
}

std::vector<uint8_t> License::EncryptBlocks(const void* data, size_t size,
                                             const std::array<uint8_t, 128>& schedule, bool decrypt)
{
    if (data == nullptr || (size & 7) != 0) {
        return {};
    }
    const auto* input = static_cast<const uint8_t*>(data);
    std::vector<uint8_t> output(size);
    for (size_t offset = 0; offset < size; offset += 8) {
        const auto block = CryptDesBlock(input + offset, schedule, decrypt);
        std::copy(block.begin(), block.end(), output.begin() + offset);
    }
    return output;
}

std::array<uint8_t, 16> License::Md5(const void* data, size_t size)
{
    if (data == nullptr && size != 0) {
        throw std::invalid_argument("MD5 input is null");
    }
    uint32_t state[4] = {0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476};
    const auto* bytes = static_cast<const uint8_t*>(data);
    for (size_t offset = 0; offset + 64 <= size; offset += 64) {
        Md5Transform(state, bytes + offset);
    }
    uint8_t final[128] = {};
    const size_t remaining = size & 63;
    if (remaining != 0) {
        std::memcpy(final, bytes + size - remaining, remaining);
    }
    final[remaining] = 0x80;
    const size_t finalSize = remaining < 56 ? 64 : 128;
    const uint64_t bitLength = static_cast<uint64_t>(size) * 8;
    for (int index = 0; index < 8; ++index) {
        final[finalSize - 8 + index] = static_cast<uint8_t>(bitLength >> (index * 8));
    }
    Md5Transform(state, final);
    if (finalSize == 128) {
        Md5Transform(state, final + 64);
    }
    std::array<uint8_t, 16> digest{};
    for (int index = 0; index < 4; ++index) {
        for (int byte = 0; byte < 4; ++byte) {
            digest[index * 4 + byte] = static_cast<uint8_t>(state[index] >> (byte * 8));
        }
    }
    return digest;
}

int License::CheckVerification(int requestedSlot) const
{
    if (checkPolicy == LicenseCheckPolicy::OfflineAuthorized) {
        return 1;
    }
    int index = 0;
    while (index < 16 && verification[index] == 1) {
        ++index;
    }
    const int expected = checkPolicy == LicenseCheckPolicy::CrackedBinary ? 9 : requestedSlot;
    return index == expected ? 1 : -index;
}

void License::SetVerification(int slot)
{
    if (slot >= 0 && slot < 16) {
        verification[slot] = 1;
    }
}

void License::ClearVerification()
{
    verification.fill(0);
}

bool License::HandleResponse(const void* data, size_t size)
{
    if (data == nullptr || size < 10 || ((size - 2) & 7) != 0) {
        return false;
    }
    const auto* bytes = static_cast<const uint8_t*>(data);
    const uint16_t response = Read16BE(bytes);
    const auto& key = sessionKeyReceived ? sessionKeySchedule : initialKeySchedule;
    const auto plain = EncryptBlocks(bytes + 2, size - 2, key, true);
    if (plain.empty()) {
        return false;
    }
    if (response == 100) {
        if (plain.size() < 128) {
            return false;
        }
        std::copy_n(plain.begin(), 128, sessionKeySchedule.begin());
        sessionKeyReceived = true;
        std::vector<uint8_t> request;
        Append16BE(request, 1);
        Append16BE(request, 1);
        const auto digest = Md5(identity.data(), identity.size());
        request.insert(request.end(), digest.begin(), digest.end());
        return SendPlain(std::move(request), false);
    }
    if (response != 101 || plain.size() < 8) {
        return false;
    }
    const int32_t operation = static_cast<int32_t>(Read32BE(plain.data()));
    const int32_t bit = static_cast<int32_t>(Read32BE(plain.data() + 4));
    SetVerification(bit);
    const int slot = ((randomSlot() % 16) + 16) % 16;
    const int check = CheckVerification(slot);
    if (operation == 2) {
        if (sceneResult) sceneResult(check == 0 ? 6 : 4);
    } else if (operation == 1) {
        if (!menuRegistered) {
            if (menuResult) menuResult(check == 0 ? 2 : 4);
        } else if (sceneResult) {
            sceneResult(check == 0 ? 2 : 4);
        }
    } else {
        return false;
    }
    if (check != 0 && plain.size() > 8 && payloadHandler) {
        payloadHandler(std::vector<uint8_t>(plain.begin() + 8, plain.end()));
    }
    return true;
}

int License::BeginSceneAuthorization()
{
    authorizationAttempts = 1;
    authorizationElapsed = CheckVerification(0) + 9;
    sceneTimeout = 2880;
    sms->Send(DecodeMessage(0), DecodeMessage(1));
    return 2;
}

int License::UpdateSceneAuthorization()
{
    if (--sceneTimeout < 0) {
        return 5;
    }
    const int status = sms->PollStatus();
    if (status == 2) {
        return 5;
    }
    if (status != 1) {
        return 2;
    }
    int check = CheckVerification(0);
    if (check < 1) {
        ++authorizationAttempts;
        SetVerification(-check);
    }
    check = CheckVerification(0);
    if (check == 1) {
        return 4;
    }
    sms->Send(DecodeMessage(0), DecodeMessage(1));
    return 2;
}

std::u16string License::DecodeMessage(int index) const
{
    if (index < 0 || index > 1) {
        return {};
    }
    const auto plain = EncryptBlocks(messages[index].data(), messages[index].size(),
                                     initialKeySchedule, true);
    std::u16string output;
    for (size_t offset = 0; offset + 1 < plain.size(); offset += 2) {
        const char16_t value = static_cast<char16_t>(plain[offset] | (plain[offset + 1] << 8));
        if (value == 0) break;
        output.push_back(value);
    }
    return output;
}

} // namespace game
