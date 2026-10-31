"""Bundle the local dashboard in flash; no filesystem image or CDN needed."""
from pathlib import Path
Import("env")
project = Path(env.subst("$PROJECT_DIR"))
output = Path(env.subst("$BUILD_DIR")) / "generated"
output.mkdir(parents=True, exist_ok=True)
page = (project / "web/index.html").read_text(encoding="utf-8")
tft_page = (project / "web/tft.html").read_text(encoding="utf-8")
assert ')ZS_WEB"' not in page
assert ')ZS_TFT"' not in tft_page
header = ('#pragma once\n#include <Arduino.h>\n'
          'static const char kWebPage[] PROGMEM = R"ZS_WEB(' + page + ')ZS_WEB";\n'
          'static const char kTftPage[] PROGMEM = R"ZS_TFT(' + tft_page + ')ZS_TFT";\n')
path = output / "web_page.hpp"
if not path.exists() or path.read_text(encoding="utf-8") != header:
    path.write_text(header, encoding="utf-8")

env.Append(CPPPATH=[str(output)])
