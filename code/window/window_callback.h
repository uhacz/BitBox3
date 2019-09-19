#pragma once

#include <stdint.h>

typedef int(*BXWin32WindowCallback)(uintptr_t hwnd, uint32_t msg, uintptr_t wparam, uintptr_t lparam, void* userdata );