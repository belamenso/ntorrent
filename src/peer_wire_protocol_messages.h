#ifndef PEER_WIRE_PROTOCOL_MESSAGES_H
#define PEER_WIRE_PROTOCOL_MESSAGES_H

#include <iostream>

enum class PeerWireProtocolMessageType: uint8_t {
    choke = 0,
    unchoke = 1,
    interested = 2,
    not_interested = 3,
    have = 4,
    bitfield = 5,
    request = 6,
    piece = 7,
    cancel = 8,
    port = 9,
};

struct PeerWireProtocolMessage {
    [[nodiscard]] virtual PeerWireProtocolMessageType type() const = 0;
    [[nodiscard]] virtual uint32_t expected_size() const = 0;
    virtual void write(uint8_t*) const = 0;
protected:
    virtual std::ostream& print(std::ostream& os) const = 0;
    friend std::ostream& operator<<(std::ostream& out, const PeerWireProtocolMessage& m) {
        return m.print(out);
    }
};

struct ChokeMessage: PeerWireProtocolMessage {
    [[nodiscard]] PeerWireProtocolMessageType type() const override { return PeerWireProtocolMessageType::choke; }

    [[nodiscard]] uint32_t expected_size() const override { return 1; }

    void write(uint8_t* data) const override {
        data[0] = static_cast<uint8_t>(this->type());
    }

    std::ostream& print(std::ostream& out) const override {
        out << "ChokeMessage[]";
        return out;
    }

    static optional<ChokeMessage> parse(const uint8_t* data, unsigned length) {
        if (length != 1) return {};
        if (static_cast<PeerWireProtocolMessageType>(data[0]) != PeerWireProtocolMessageType::choke) return {};
        return ChokeMessage();
    }
};

struct UnchokeMessage: PeerWireProtocolMessage {
    [[nodiscard]] PeerWireProtocolMessageType type() const override { return PeerWireProtocolMessageType::unchoke; }

    [[nodiscard]] uint32_t expected_size() const override { return 1; }

    void write(uint8_t* data) const override {
        data[0] = static_cast<uint8_t>(this->type());
    }

    std::ostream& print(std::ostream& out) const override {
        out << "UnchokeMessage[]";
        return out;
    }

    static optional<UnchokeMessage> parse(const uint8_t* data, unsigned length) {
        if (length != 1) return {};
        if (static_cast<PeerWireProtocolMessageType>(data[0]) != PeerWireProtocolMessageType::unchoke) return {};
        return UnchokeMessage();
    }
};

struct InterestedMessage: PeerWireProtocolMessage {
    [[nodiscard]] PeerWireProtocolMessageType type() const override { return PeerWireProtocolMessageType::interested; }

    [[nodiscard]] uint32_t expected_size() const override { return 1; }

    void write(uint8_t* data) const override {
        data[0] = static_cast<uint8_t>(this->type());
    }

    std::ostream& print(std::ostream& out) const override {
        out << "InterestedMessage[]";
        return out;
    }

    static optional<InterestedMessage> parse(const uint8_t* data, unsigned length) {
        if (length != 1) return {};
        if (static_cast<PeerWireProtocolMessageType>(data[0]) != PeerWireProtocolMessageType::interested) return {};
        return InterestedMessage();
    }
};

struct NotInterestedMessage: PeerWireProtocolMessage {
    [[nodiscard]] PeerWireProtocolMessageType type() const override { return PeerWireProtocolMessageType::not_interested; }

    [[nodiscard]] uint32_t expected_size() const override { return 1; }

    void write(uint8_t* data) const override {
        data[0] = static_cast<uint8_t>(this->type());
    }

    std::ostream& print(std::ostream& out) const override {
        out << "NotInterestedMessage[]";
        return out;
    }

    static optional<NotInterestedMessage> parse(const uint8_t* data, unsigned length) {
        if (length != 1) return {};
        if (static_cast<PeerWireProtocolMessageType>(data[0]) != PeerWireProtocolMessageType::not_interested) return {};
        return NotInterestedMessage();
    }
};

struct HaveMessage: PeerWireProtocolMessage {
    const uint32_t piece_index;

    explicit HaveMessage(uint32_t piece_index) : piece_index(piece_index) {}

    [[nodiscard]] PeerWireProtocolMessageType type() const override { return PeerWireProtocolMessageType::have; }

    [[nodiscard]] uint32_t expected_size() const override { return 5; }

    void write(uint8_t* data) const override {
        data[0] = static_cast<uint8_t>(this->type());
        reinterpret_cast<uint32_t*>(data+1)[0] = htonl(this->piece_index);
    }

    std::ostream& print(std::ostream& out) const override {
        out << "HaveMessage[piece_index=" << piece_index << "]";
        return out;
    }

    static optional<HaveMessage> parse(const uint8_t* data, unsigned length) {
        if (length != 5) return {};
        if (static_cast<PeerWireProtocolMessageType>(data[0]) != PeerWireProtocolMessageType::have) return {};
        return HaveMessage(ntohl(*reinterpret_cast<const uint32_t*>(data+1)));
    }
};

struct BitfieldMessage: PeerWireProtocolMessage {
    const vector<bool> bitfield;

    explicit BitfieldMessage(vector<bool> bitfield) : bitfield(std::move(bitfield)) {}

    [[nodiscard]] PeerWireProtocolMessageType type() const override { return PeerWireProtocolMessageType::bitfield; }

    [[nodiscard]] uint32_t expected_size() const override { return 1 + (bitfield.size()+7)/8; }

    void write(uint8_t* data) const override {
        data[0] = static_cast<uint8_t>(this->type());
        for (int i = 0; i < bitfield.size(); i += 8) {
            if (i+7 < bitfield.size()) {
                data[1+i/8] = (
                        (bitfield[i+0] ? 0b10000000 : 0) |
                        (bitfield[i+1] ? 0b01000000 : 0) |
                        (bitfield[i+2] ? 0b00100000 : 0) |
                        (bitfield[i+3] ? 0b00010000 : 0) |
                        (bitfield[i+4] ? 0b00001000 : 0) |
                        (bitfield[i+5] ? 0b00000100 : 0) |
                        (bitfield[i+6] ? 0b00000010 : 0) |
                        (bitfield[i+7] ? 0b00000001 : 0) );
            } else {
                data[1+i/8] = (
                        (bitfield[i+0] ? 0b10000000 : 0) |
                        (bitfield[i+1] and i+1 < bitfield.size() ? 0b01000000 : 0) |
                        (bitfield[i+2] and i+2 < bitfield.size() ? 0b00100000 : 0) |
                        (bitfield[i+3] and i+3 < bitfield.size() ? 0b00010000 : 0) |
                        (bitfield[i+4] and i+4 < bitfield.size() ? 0b00001000 : 0) |
                        (bitfield[i+5] and i+5 < bitfield.size() ? 0b00000100 : 0) |
                        (bitfield[i+6] and i+6 < bitfield.size() ? 0b00000010 : 0) |
                        (bitfield[i+7] and i+7 < bitfield.size() ? 0b00000001 : 0) );
            }
        }
    }

    std::ostream& print(std::ostream& out) const override {
        out << "BitfieldMessage[bitfield=(vector of size " << bitfield.size() << "){";
        for (const bool b: bitfield) out << int(b);
        out << "}]";
        return out;
    }

    static optional<BitfieldMessage> parse(const uint8_t* data, unsigned length, unsigned number_of_bits) {
        if (length != 1 + (number_of_bits+7)/8) return {};
        vector<bool> bs(8*((number_of_bits+7)/8), false);
        if (static_cast<PeerWireProtocolMessageType>(data[0]) != PeerWireProtocolMessageType::bitfield) return {};

        for (int i = 0; i < length-1; i++) {
            bs[8*i+0] = !!(data[1+i] & 0b10000000);
            bs[8*i+1] = !!(data[1+i] & 0b01000000);
            bs[8*i+2] = !!(data[1+i] & 0b00100000);
            bs[8*i+3] = !!(data[1+i] & 0b00010000);
            bs[8*i+4] = !!(data[1+i] & 0b00001000);
            bs[8*i+5] = !!(data[1+i] & 0b00000100);
            bs[8*i+6] = !!(data[1+i] & 0b00000010);
            bs[8*i+7] = !!(data[1+i] & 0b00000001);
        }

        while (bs.size() > number_of_bits) {
            if (bs[bs.size()-1]) return {};
            bs.pop_back();
        }
        return BitfieldMessage(std::move(bs));
    }
};

struct RequestMessage: PeerWireProtocolMessage {
    const uint32_t index, begin, length;

    RequestMessage(uint32_t index, uint32_t begin, uint32_t length)
            : index(index), begin(begin), length(length) {}

    [[nodiscard]] PeerWireProtocolMessageType type() const override { return PeerWireProtocolMessageType::request; }

    [[nodiscard]] uint32_t expected_size() const override { return 1+3*4; }

    void write(uint8_t* data) const override {
        data[0] = static_cast<uint8_t>(this->type());
        reinterpret_cast<uint32_t*>(data+1)[0] = htonl(this->index);
        reinterpret_cast<uint32_t*>(data+1)[1] = htonl(this->begin);
        reinterpret_cast<uint32_t*>(data+1)[2] = htonl(this->length);
    }

    std::ostream& print(std::ostream& out) const override {
        out << "RequestMessage[index=" << index << ", begin=" << begin << ", length=" << length << "]";
        return out;
    }

    static optional<RequestMessage> parse(const uint8_t* data, unsigned length) {
        if (length != 13) return {};
        if (static_cast<PeerWireProtocolMessageType>(data[0]) != PeerWireProtocolMessageType::request) return {};
        return RequestMessage(
                ntohl(reinterpret_cast<const uint32_t*>(data+1)[0]),
                ntohl(reinterpret_cast<const uint32_t*>(data+1)[1]),
                ntohl(reinterpret_cast<const uint32_t*>(data+1)[2]));
    }
};

struct PieceMessage: PeerWireProtocolMessage {
    const uint32_t index, begin;
    const string block;

    PieceMessage(uint32_t index, uint32_t begin, string block)
            : index(index), begin(begin), block(std::move(block)) {}

    [[nodiscard]] PeerWireProtocolMessageType type() const override { return PeerWireProtocolMessageType::piece; }

    [[nodiscard]] uint32_t expected_size() const override { return 9+this->block.size(); }

    void write(uint8_t* data) const override {
        data[0] = static_cast<uint8_t>(this->type());
        reinterpret_cast<uint32_t*>(data+1)[0] = htonl(this->index);
        reinterpret_cast<uint32_t*>(data+1)[1] = htonl(this->begin);
        for (int i = 0; i < this->block.size(); i++) {
            data[9+i] = this->block[i];
        }
    }

    std::ostream& print(std::ostream& out) const override {
        out << "PieceMessage[index=" << index << ", begin=" << begin
            << ", block= (string of size " << block.size() << "){" << block << "}]";
        return out;
    }

    static optional<PieceMessage> parse(const uint8_t* data, unsigned length) {
        if (!(length >= 9)) return {};
        if (static_cast<PeerWireProtocolMessageType>(data[0]) != PeerWireProtocolMessageType::piece) return {};
        string str(length-9, '\0');
        for (int i = 0; i < str.size(); i++) str[i] = data[9+i];
        return PieceMessage(
                ntohl(reinterpret_cast<const uint32_t*>(data+1)[0]),
                ntohl(reinterpret_cast<const uint32_t*>(data+1)[1]),
                std::move(str));
    }
};

struct CancelMessage: PeerWireProtocolMessage {
    const uint32_t index, begin, length;

    CancelMessage(uint32_t index, uint32_t begin, uint32_t length)
    : index(index), begin(begin), length(length) {}

    [[nodiscard]] PeerWireProtocolMessageType type() const override { return PeerWireProtocolMessageType::cancel; }

    [[nodiscard]] uint32_t expected_size() const override { return 1+3*4; }

    void write(uint8_t* data) const override {
        data[0] = static_cast<uint8_t>(this->type());
        reinterpret_cast<uint32_t*>(data+1)[0] = htonl(this->index);
        reinterpret_cast<uint32_t*>(data+1)[1] = htonl(this->begin);
        reinterpret_cast<uint32_t*>(data+1)[2] = htonl(this->length);
    }

    std::ostream& print(std::ostream& out) const override {
        out << "CancelMessage[index=" << index << ", begin=" << begin << ", length=" << length << "]";
        return out;
    }

    static optional<CancelMessage> parse(const uint8_t* data, unsigned length) {
        if (length != 13) return {};
        if (static_cast<PeerWireProtocolMessageType>(data[0]) != PeerWireProtocolMessageType::cancel) return {};
        return CancelMessage(
                ntohl(reinterpret_cast<const uint32_t*>(data+1)[0]),
                ntohl(reinterpret_cast<const uint32_t*>(data+1)[1]),
                ntohl(reinterpret_cast<const uint32_t*>(data+1)[2]));
    }
};

struct PortMessage: PeerWireProtocolMessage {
    const uint16_t port;

    explicit PortMessage(uint16_t port) : port(port) {}

    [[nodiscard]] PeerWireProtocolMessageType type() const override { return PeerWireProtocolMessageType::port; }

    [[nodiscard]] uint32_t expected_size() const override { return 1+2; }

    void write(uint8_t* data) const override {
        data[0] = static_cast<uint8_t>(this->type());
        reinterpret_cast<uint16_t*>(data+1)[0] = htons(this->port);
    }

    std::ostream& print(std::ostream& out) const override {
        out << "PortMessage[port=" << port << "]";
        return out;
    }

    static optional<PortMessage> parse(const uint8_t* data, unsigned length) {
        if (length != 3) return {};
        if (static_cast<PeerWireProtocolMessageType>(data[0]) != PeerWireProtocolMessageType::port) return {};
        return PortMessage(ntohs(reinterpret_cast<const uint16_t*>(data+1)[0]));
    }
};

#endif
