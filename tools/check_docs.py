#!/usr/bin/env python3
"""Check local guide links and that documented dependency versions match CI."""
from pathlib import Path
import re
from urllib.parse import unquote, urlsplit

ROOT = Path(__file__).resolve().parent.parent
errors = []
workflow = (ROOT / ".github/workflows/ci.yml").read_text()
for key in ("ESP32_CORE", "LOVYANGFX", "ARDUINOJSON", "WIFIMANAGER"):
    match = re.search(rf'^  {key}: "([^"]+)"$', workflow, re.MULTILINE)
    if not match:
        errors.append(f"CI version missing: {key}")
        continue
    version = match.group(1)
    for name in ("README.md", "docs/02-install-arduino.md", "docs/development.md"):
        if version not in (ROOT / name).read_text():
            errors.append(f"{name}: missing CI version {key}={version}")

# Check the simple inline local links/images used by these guides. External
# services are deliberately not contacted by CI. Anchor fragments are checked
# manually when changing headings; this check validates file destinations.
files = [ROOT / "README.md", *sorted((ROOT / "docs").glob("*.md"))]
for path in files:
    for target in re.findall(r'\]\(([^)]+)\)', path.read_text()):
        url = urlsplit(target.strip("<>"))
        if url.scheme or url.netloc or not url.path:
            continue
        dest = (path.parent / unquote(url.path)).resolve()
        if not dest.exists():
            errors.append(f"{path.relative_to(ROOT)}: missing link target {target}")

if errors:
    raise SystemExit("\n".join(errors))
print(f"Guide checks passed: {len(files)} Markdown files and four dependency versions.")
