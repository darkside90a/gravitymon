/*
MIT License

Copyright (c) 2021-22 Magnus

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
 */
#if defined(ESP8266)
#define INCBIN_OUTPUT_SECTION ".irom.text"
#include <incbin.h>

#include <resources.hpp>
INCBIN(IndexHtm, "html/index.min.htm");
INCBIN(ConfigHtm, "html/config.min.htm");
INCBIN(CalibrationHtm, "html/calibration.min.htm");
INCBIN(FormatHtm, "html/format.min.htm");
INCBIN(TestHtm, "html/test.min.htm");
INCBIN(AboutHtm, "html/about.min.htm");
INCBIN(FirmwareHtm, "html/firmware.min.htm");
INCBIN(BackupHtm, "html/backup.min.htm");
#endif

// EOF
