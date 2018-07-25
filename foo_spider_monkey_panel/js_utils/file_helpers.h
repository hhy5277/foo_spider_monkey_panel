#pragma once

#include <optional>
#include <string>

namespace mozjs::file
{

std::optional<std::wstring> ReadFromFile( JSContext* cx, const pfc::string8_fast& path, uint32_t codepage = 0 );

}
