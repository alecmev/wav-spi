#pragma once

#define PAGESIZE 256
#define PAGEBITS 2048

#define CS   0x10
#define MISO 0x20
#define MOSI 0x02

enum class Buttons {
    None = 0,
    Refresh = 1,
    Upload = 2,
    Verify = 4,
    Step1 = 1,
    Step2 = 7
};
