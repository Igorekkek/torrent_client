#pragma once

#include <string>

int BytesToInt(std::string_view bytes);
std::string IntToBytes(int value);

std::string CalculateSHA1(const std::string& msg);

std::string HexEncode(const std::string& input);
