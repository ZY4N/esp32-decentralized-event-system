from cryptography.hazmat.backends import default_backend
from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.primitives.asymmetric import ec
from cryptography.hazmat.primitives.asymmetric import ec


def generate_key_pair():
    private_key = ec.generate_private_key(ec.SECP256R1(), default_backend())

    private_key_bytes = private_key.private_numbers().private_value.to_bytes(32, 'big')

    public_key_bytes = private_key.public_key().public_bytes(
        encoding=serialization.Encoding.X962,
        format=serialization.PublicFormat.UncompressedPoint
    )

    return private_key_bytes, public_key_bytes


def bytes_to_cpp_array(bytes):
    bytes_hex  = ", ".join(f"0x{b:02x}" for b in bytes)
    return f"std::array<std::uint8_t, {len(bytes)}>{{{bytes_hex}}}"


def gen_key_headers(count):

    private_keys = []
    public_keys = ""
    for sender_id in range(count):
        private, public = generate_key_pair()
        private_keys.append(bytes_to_cpp_array(private))
        public_keys += bytes_to_cpp_array(public) + ",\n";

    for sender_id in range(count):
        header = f"""#pragma once
#include <array>
#include <cinttypes>

inline constexpr auto private_key = {private_keys[sender_id]};

inline constexpr auto public_keys = std::array{{{public_keys}}};
"""
        with open(f"main/include/keys/event_keys_{sender_id}.hpp", "w") as text_file:
            text_file.write(header)


gen_key_headers(10)
